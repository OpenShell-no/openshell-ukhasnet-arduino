#!/usr/bin/env python2.7
# -*- coding: utf8 -*-
# Copyright (c) 2016 Odd Strabo <oddstr13@openshell.no>
# License: MIT - https://opensource.org/licenses/MIT

VERSION_HEADER_FILE = "firmware_version.h"

TEMPLATE = """
#ifndef FIRMWARE_VERSION_H
#define FIRMWARE_VERSION_H
#pragma message "Firmware version is {version} at commit {commit} in branch {branch}"
const char firmware_version[] = "{version}";
const char firmware_commit[] = "{commit}";
const bool firmware_commit_dirty = {dirty};
const char firmware_branch[] = "{branch}";
const char firmware_date[] = "{date}";
#endif
"""

import os
import datetime
import traceback

def generate(ui, repo, **kwargs):
    try:
        variables = {}
        ctx = repo[None]

        dirty = not not ctx.dirty()

        parents = sorted(ctx.parents(), key=lambda x: x.rev(), reverse=True)
        ctx = parents[0]


        variables['branch'] = ctx.branch()
        version = ""
        if dirty:
            variables['commit'] = "{commit}+".format(commit=ctx.hex())
            if not version:
                version = "0.0.{revision}+".format(revision=ctx.rev())
            variables['dirty'] = 'true'
        else:
            variables['commit'] = ctx.hex()
            if not version:
                version = "0.0.{revision}".format(revision=ctx.rev())

            variables['dirty'] = 'false'

        d = ctx.date()
        variables['date'] = (datetime.datetime.fromtimestamp(int(d[0])) + datetime.timedelta(seconds=d[1])).isoformat() + 'Z'

        tags = ctx.tags() # TODO:70 make function to locate latest x.y.z tag in parents, and use as basis of version
        while 'tip' in tags: tags.remove('tip')
        print(tags)

        variables['version'] = version

        output = TEMPLATE.format(**variables)
        if os.path.exists(VERSION_HEADER_FILE) and open(VERSION_HEADER_FILE, "rb").read() != output or not os.path.exists(VERSION_HEADER_FILE):
            print(variables)
            with open(VERSION_HEADER_FILE, "wb") as fh:
                fh.write(output)
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
