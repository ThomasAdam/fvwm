#!/bin/sh

# 2011-06-25:  Thomas Adam <thomas@fvwm.org>
# Some of this was borrowed from git.git's GIT-VERSION-GEN file.

# This determines the version of FVWM we're running from git, or if not the
# released tarball.

DEF_VER=2.6.1
LF='
'

if test -d .git &&
    VN=$(git describe --tags --match "version-[0-9]*" --abbrev=7 HEAD 2>/dev/null) &&
    case "$VN" in
    *$LF*) (exit 1) ;;
    version-[0-9]*)
        git update-index -q --refresh
        test -z "$(git diff-index --name-only HEAD --)" ||
        VN="$VN-dirty" ;;
    esac
then
    VN=$(echo "$VN" | sed -e 's/[0-9]+-/./g');
else
    VN="$DEF_VER"
fi

# Tag names are in the form:
#
# version-2_5_0
#
# we should strip that, and convert the underscores to spaces.
VN=${VN##version-}
VN=$(echo "$VN" | sed -e 's/_/\./g')

echo "m4_define([FVWM_VERSION], [$VN])"
