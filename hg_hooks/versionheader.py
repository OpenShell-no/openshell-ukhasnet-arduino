#!/usr/bin/env python
# -*- coding: utf8 -*-
# Copyright (c) 2016 Odd Stråbø <oddstr13@openshell.no>
# License: MIT - https://opensource.org/licenses/MIT

VERSION_HEADER_FILE = "firmware_version.h"

TEMPLATE = """
#ifndef FIRMWARE_VERSION_H
#define FIRMWARE_VERSION_H
#pragma message "Firmware version is {version} at commit {commit} in branch {branch}"
const char firmware_version[] = "{version}";
const char firmware_commit[] = "{commit}";
const char firmware_branch[] = "{branch}";
const char firmware_date[] = "{date}";
#endif
"""

import datetime
import traceback

def generate(ui, repo, **kwargs):
    try:
        variables = {}
        ctx = repo[None]

        dirty = not not ctx.dirty()

        variables['branch'] = ctx.branch()
        version = ""
        if dirty:
            parents = sorted(ctx.parents(), key=lambda x: x.rev(), reverse=True)
            parent = parents[0]
            variables['commit'] = "{commit}+".format(commit=parent.hex())
            if not version:
                version = "0.0.{revision}+".format(revision=parent.rev())
        else:
            variables['commit'] = ctx.hex()
            if not version:
                version = "0.0.{revision}".format(revision=ctx.rev())

        d = ctx.date()
        variables['date'] = (datetime.datetime.fromtimestamp(int(d[0])) + datetime.timedelta(seconds=d[1])).isoformat() + 'Z'

        tags = ctx.tags()

        variables['version'] = version
        print(variables)

        with open(VERSION_HEADER_FILE, "wb") as fh:
            fh.write(TEMPLATE.format(**variables))
    except:
        traceback.print_exc()
        raise

def main():
    import os
    repopath = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))

    from mercurial import ui, hg
    u=ui.ui()
    repo = hg.repository(u, repopath)

    os.chdir(repopath)
    generate(u, repo)

if __name__ == "__main__":
    main()
