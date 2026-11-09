#!/usr/bin/env python3
"""
知识库健康检查（断链 / frontmatter / 同名冲突 / 内容质量）

用法（在知识库根目录）：
    python 06_工具箱/知识库检查.py

契约依据：AI导航.md 第 2 节（frontmatter）、第 5 节（链接规范）
【1】~【3】为结构检查，计入退出码；【4】为内容质量建议项（对话腔 /
空标签 / 停滞笔记 / 空壳笔记），只提示不计入退出码。
"""
import datetime
import os
import re
import sys
from collections import defaultdict

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SKIP_DIRS = {".git", ".workbuddy", ".trae", ".obsidian"}
TYPES = {"项目", "知识", "板卡", "来源", "排坑", "竞赛", "课程", "工具", "代码", "索引", "个人档案", "进度", "清单"}
STATES = {"进行中", "已完成", "归档"}
LINK_EXTS = {".md", ".c", ".h", ".png", ".jpg", ".jpeg", ".xlsx", ".kmodel", ".pdf", ".svg"}

# 模板 / 入口文件不参与 frontmatter 检查
FM_EXEMPT = {"AGENTS.md", "README.md", "AI导航.md", "知识库上手指南_给师弟.md"}  # 末项为对外分享文档（AI导航 2.3）
FM_EXEMPT_PREFIX = ("07_模板/", "07_模板\\", ".trae/")

LINK_RE = re.compile(r"\[\[([^\]\|#]+)")
FENCE_RE = re.compile(r"^\s*```")
INLINE_CODE_RE = re.compile(r"`[^`]*`")
FM_KEY_RE = re.compile(r"^([\u4e00-\u9fa5A-Za-z_]+):")

# 【4】内容质量：对话腔关键词（笔记必须脱离对话上下文可独立阅读）
CHAT_WORDS = ["如上所述", "上文", "刚才", "前面提到", "我们之前", "你提到",
              "对话中", "聊天记录", "希望对你有帮助"]
# 进行中超过该天数视为停滞，周归档时应关闭或拆分
STALE_DAYS = 60
# 永久维护的活文档不参与停滞检查（索引/规则文件）
STALE_EXEMPT = {"首页.md", "00_收件箱/使用说明.md"}
# 知识 / 排坑类正文少于该非空行数视为空壳
STUB_LINES = 10
# 内容质量检查跳过模板与已归档内容
QUALITY_SKIP_PREFIX = ("07_模板/", "08_归档/")


def iter_files():
    for dirpath, dirnames, filenames in os.walk(ROOT):
        dirnames[:] = [d for d in dirnames if d not in SKIP_DIRS]
        for name in filenames:
            ext = os.path.splitext(name)[1].lower()
            if ext in LINK_EXTS:
                full = os.path.join(dirpath, name)
                rel = os.path.relpath(full, ROOT).replace("\\", "/")
                yield rel, name, ext


def strip_code(text):
    """去掉围栏代码块与行内反引号，只保留会被 Obsidian 解析成链接的正文"""
    out = []
    in_fence = False
    for line in text.splitlines():
        if FENCE_RE.match(line):
            in_fence = not in_fence
            continue
        if in_fence:
            continue
        out.append(INLINE_CODE_RE.sub("", line))
    return "\n".join(out)


def parse_frontmatter(raw):
    """返回 (frontmatter 字典, 正文起始行号)。无 frontmatter 时返回 ({}, 0)。"""
    lines = raw.splitlines()
    fm = {}
    if not lines or lines[0].strip() != "---":
        return fm, 0
    for i, line in enumerate(lines[1:], start=1):
        if line.strip() == "---":
            return fm, i + 1
        m = FM_KEY_RE.match(line)
        if m:
            fm[m.group(1)] = line.split(":", 1)[1].strip()
    return fm, 0


def main():
    print(f"知识库：{ROOT}\n")

    names = set()
    md_files = []
    clash = defaultdict(list)
    for rel, name, ext in iter_files():
        stem = os.path.splitext(name)[0]
        names.add(stem)
        if ext == ".md":
            md_files.append(rel)
            clash[name].append(rel)

    # ---------- 1. 断链 ----------
    print("【1】断链检查")
    broken = 0
    for rel in md_files:
        with open(os.path.join(ROOT, rel), encoding="utf-8", errors="ignore") as fh:
            body = strip_code(fh.read())
        for target in sorted(set(LINK_RE.findall(body))):
            stem = os.path.splitext(os.path.basename(target.strip()))[0]
            if stem and stem not in names:
                print(f"  x {rel} -> [[{target}]]")
                broken += 1
    print("  OK 无断链" if broken == 0 else f"  共 {broken} 处断链")
    print()

    # ---------- 2. frontmatter ----------
    print("【2】frontmatter 合规")
    bad = 0
    for rel in md_files:
        base = os.path.basename(rel)
        if base in FM_EXEMPT or base.endswith(".excalidraw.md") or rel.startswith(FM_EXEMPT_PREFIX):
            continue
        with open(os.path.join(ROOT, rel), encoding="utf-8", errors="ignore") as fh:
            lines = fh.read().splitlines()
        if not lines or lines[0].strip() != "---":
            print(f"  x {rel} -> 缺 frontmatter")
            bad += 1
            continue
        fm = {}
        for line in lines[1:]:
            if line.strip() == "---":
                break
            m = FM_KEY_RE.match(line)
            if m:
                fm[m.group(1)] = line.split(":", 1)[1].strip().strip("[]")
        for key in ("类型", "状态", "标签", "创建"):
            if key not in fm:
                print(f"  x {rel} -> 缺 {key}")
                bad += 1
        if fm.get("类型") and fm["类型"] not in TYPES:
            print(f"  x {rel} -> 类型「{fm['类型']}」不合法")
            bad += 1
        if fm.get("状态") and fm["状态"] not in STATES:
            print(f"  x {rel} -> 状态「{fm['状态']}」不合法")
            bad += 1
        legacy = {"type", "status", "trust_level", "platform", "mcu", "package", "created"} & set(fm)
        if legacy:
            print(f"  x {rel} -> 残留英文键 {sorted(legacy)}")
            bad += 1
    print("  OK 全部合规" if bad == 0 else f"  共 {bad} 处问题")
    print()

    # ---------- 3. 同名冲突 ----------
    print("【3】同名文件冲突（[[短链]] 会歧义）")
    dups = {k: v for k, v in clash.items() if len(v) > 1 and k != "README.md"}
    if not dups:
        print("  OK 无冲突")
    else:
        for k, v in sorted(dups.items()):
            print(f"  x {k}: " + " | ".join(v))
    print()
    # ---------- 4. 内容质量（建议项，不计入退出码） ----------
    print("【4】内容质量（建议项）")
    warn = 0
    today = datetime.date.today()
    for rel in md_files:
        if rel.startswith(QUALITY_SKIP_PREFIX):
            continue
        with open(os.path.join(ROOT, rel), encoding="utf-8", errors="ignore") as fh:
            raw = fh.read()
        fm, body_start = parse_frontmatter(raw)

        hits = set()
        for line in strip_code(raw).splitlines():
            if "禁止" in line or "不许" in line:  # 红线规则引用这些词作示例，不算对话腔
                continue
            hits.update(w for w in CHAT_WORDS if w in line)
        if hits:
            print(f"  ! {rel} -> 对话腔残留 {sorted(hits)}")
            warn += 1
        if fm.get("标签") == "[]":
            print(f"  ! {rel} -> 空标签")
            warn += 1
        if fm.get("状态") == "进行中" and fm.get("创建") and rel not in STALE_EXEMPT and fm.get("类型") != "索引":
            try:
                created = datetime.datetime.strptime(fm["创建"], "%Y-%m-%d").date()
                age = (today - created).days
                if age > STALE_DAYS:
                    print(f"  ! {rel} -> 进行中已 {age} 天，周归档时关闭或拆分")
                    warn += 1
            except ValueError:
                pass
        if fm.get("类型") in ("知识", "排坑"):
            body_lines = sum(1 for line in raw.splitlines()[body_start:] if line.strip())
            if body_lines < STUB_LINES:
                print(f"  ! {rel} -> 正文仅 {body_lines} 行，疑似空壳")
                warn += 1
    print("  OK 无腐化迹象" if warn == 0 else f"  共 {warn} 处建议（不计入退出码）")
    print()
    print("检查完成。")
    return 1 if (broken or bad or dups) else 0


if __name__ == "__main__":
    sys.exit(main())
