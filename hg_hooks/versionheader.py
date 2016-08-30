VERSION_HEADER_FILE = "firmware_version.h"

TEMPLATE = """
#ifndefined FIRMWARE_VERSION_H
#define FIRMWARE_VERSION_H
#pragma message "Firmware version is {version} at commit {commit}"
const char[] firmware_version = F("{version}");
const char[] firmware_commit = F("{commit}");
const char[] firmware_branch = F("{branch}");
#endif
"""

def generate(ui, repo, **kwargs):
    variables = {}
    ctx = repo[None]
    variables['commit'] = ctx.hex()
    date = ctx.date()
    print([type(date), date])
    tags = ctx.tags()
    print([type(tags), tags])
    version = ""
