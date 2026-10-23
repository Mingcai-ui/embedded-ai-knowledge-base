#!/usr/bin/env python3
"""
知识库健康检查（断链 / frontmatter / 同名冲突）

用法（在知识库根目录）：
    python 06_工具箱/知识库检查.py

契约依据：AI导航.md 第 2 节（frontmatter）、第 5 节（链接规范）
"""
import os
import re
import sys
from collections import defaultdict

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SKIP_DIRS = {".git", ".workbuddy", ".trae", ".obsidian"}
TYPES = {"项目", "知识", "板卡", "来源", "排坑", "竞赛", "课程", "工具", "代码", "索引", "个人档案"}
STATES = {"进行中", "已完成", "归档"}
LINK_EXTS = {".md", ".c", ".h", ".png", ".jpg", ".jpeg", ".xlsx", ".kmodel", ".pdf", ".svg"}

# 模板 / 入口文件不参与 frontmatter 检查
FM_EXEMPT = {"AGENTS.md", "README.md", "AI导航.md"}
FM_EXEMPT_PREFIX = ("07_模板/", "07_模板\\", ".trae/")

LINK_RE = re.compile(r"\[\[([^\]\|#]+)")
FENCE_RE = re.compile(r"^\s*```")
INLINE_CODE_RE = re.compile(r"`[^`]*`")
FM_KEY_RE = re.compile(r"^([\u4e00-\u9fa5A-Za-z_]+):")


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
    print("检查完成。")
    return 1 if (broken or bad or dups) else 0


if __name__ == "__main__":
    sys.exit(main())
