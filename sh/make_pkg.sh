#!/bin/sh
#                     M A K E _ P K G . S H
# BRL-CAD
#
# Copyright (c) 2005 United States Government as represented by
# the U.S. Army Research Laboratory.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above 
# copyright notice, this list of conditions and the following
# disclaimer in the documentation and/or other materials provided
# with the distribution.
#
# 3. The name of the author may not be used to endorse or promote
# products derived from this software without specific prior written
# permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
# OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
# GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
###
#
# Script for generating a Mac OS X Installer package (.pkg) from a
# clean package installation.  The package should be compatible with
# old and new versions of Installer.
#
# Author: Christopher Sean Morrison
#
######################################################################

NAME="$1"
MAJOR_VERSION="$2"
MINOR_VERSION="$3"
PATCH_VERSION="$4"
ARCHIVE="$5"
if [ "x$NAME" = "x" ] ; then
    echo "Usage: $0 title major_version minor_version patch_version archive_dir"
    echo "ERROR: must specify a package name"
    exit 1
fi
if [ "x$MINOR_VERSION" = "x" ] ; then
    echo "Usage: $0 title major_version minor_version patch_version archive_dir"
    echo "ERROR: must specify a major package version"
    exit 1
fi
if [ "x$MINOR_VERSION" = "x" ] ; then
    echo "ERROR: must specify a minor package version"
    echo "Usage: $0 title major_version minor_version patch_version archive_dir"
    exit 1
fi
if [ "x$PATCH_VERSION" = "x" ] ; then
    echo "Usage: $0 title major_version minor_version patch_version archive_dir"
    echo "ERROR: must specify a patch package version"
    exit 1
fi
if [ "x$ARCHIVE" = "x" ] ; then
    echo "Usage: $0 title major_version minor_version patch_version archive_dir"
    echo "ERROR: must specify an archive directory"
    exit 1
fi
if [ ! -d "$ARCHIVE" ] ; then
    echo "ERROR: specified archive path (${ARCHIVE}) is not a directory"
    exit 1
fi

VERSION="${MAJOR_VERSION}.${MINOR_VERSION}.${PATCH_VERSION}"
PKG_NAME="${NAME}-${VERSION}"
mkdir "${PKG_NAME}.pkg"
if [ ! -d "${PKG_NAME}.pkg" ] ; then
    echo "ERROR: unable to create the package directory"
    exit 1
fi

mkdir "${PKG_NAME}.pkg/Contents"
if [ ! -d "${PKG_NAME}.pkg/Contents" ] ; then
    echo "ERROR: unable to create the package contents directory"
    exit 1
fi

mkdir "${PKG_NAME}.pkg/Contents/Resources"
if [ ! -d "${PKG_NAME}.pkg/Contents/Resources" ] ; then
    echo "ERROR: unable to create the package resources directory"
    exit 1
fi

cat > "${PKG_NAME}.pkg/Contents/PkgInfo" <<EOF
pmkrpkg1
EOF
if [ ! -f "${PKG_NAME}.pkg/Contents/PkgInfo" ] ; then
    echo "ERROR: unable to create PkgInfo file"
    exit 1
fi

cat > "${PKG_NAME}.pkg/Contents/Info.plist" <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple Computer//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
        <key>CFBundleGetInfoString</key>
        <string>${NAME} ${VERSION}</string>
        <key>CFBundleIdentifier</key>
        <string>org.brlcad.${NAME}</string>
        <key>CFBundleName</key>
        <string>${NAME}</string>
        <key>CFBundleShortVersionString</key>
        <string>${MAJOR_VERSION}.${MINOR_VERSION}</string>
        <key>IFMajorVersion</key>
        <integer>${MAJOR_VERSION}</integer>
        <key>IFMinorVersion</key>
        <integer>${MINOR_VERSION}</integer>
        <key>IFPkgFlagAllowBackRev</key>
        <true/>
        <key>IFPkgFlagAuthorizationAction</key>
        <string>AdminAuthorization</string>
        <key>IFPkgFlagDefaultLocation</key>
        <string>/</string>
        <key>IFPkgFlagInstallFat</key>
        <false/>
        <key>IFPkgFlagIsRequired</key>
        <true/>
        <key>IFPkgFlagRelocatable</key>
        <false/>
        <key>IFPkgFlagRestartAction</key>
        <string>NoRestart</string>
        <key>IFPkgFlagRootVolumeOnly</key>
        <false/>
        <key>IFPkgFlagUpdateInstalledLanguages</key>
        <false/>
        <key>IFPkgFormatVersion</key>
        <real>0.10000000149011612</real>
</dict>
</plist>
EOF
if [ ! -f "${PKG_NAME}.pkg/Contents/Info.plist" ] ; then
    echo "ERROR: unable to create Info.plist file"
    exit 1
fi

pax -w -f "${PKG_NAME}.pkg/Contents/Archive.pax" "$ARCHIVE"
if [ $? != 0 ] ; then
    echo "ERROR: unable to successfully create a pax archive of $ARCHIVE"
    exit 1
fi
if [ ! -f "${PKG_NAME}.pkg/Contents/Archive.pax" ] ; then
    echo "ERROR: pax archive does not exist"
    exit 1
fi

gzip -c "${PKG_NAME}.pkg/Contents/Archive.pax" > "${PKG_NAME}.pkg/Contents/Archive.pax.gz"
if [ $? != 0 ] ; then
    echo "ERROR: unable to successfully compress the pax archive"
    exit 1
fi
if [ ! -f "${PKG_NAME}.pkg/Contents/Archive.pax.gz" ] ; then
    echo "ERROR: compressed pax archive does not exist"
    exit 1
fi

rm -f "${PKG_NAME}.pkg/Contents/Archive.pax"
if [ -f "${PKG_NAME}.pkg/Contents/Archive.pax" ] ; then
    echo "ERROR: unable to remove uncompressed pax archive"
    exit 1
fi

mkbom "$ARCHIVE" "${PKG_NAME}.pkg/Contents/Archive.bom"
if [ $? != 0 ] ; then
    echo "ERROR: unable to successfully generate a bill of materials"
    exit 1
fi
if [ ! -f "${PKG_NAME}.pkg/Contents/Archive.bom" ] ; then
    echo "ERROR: bill of materials file does not exist"
    exit 1
fi

NUM_FILES=`find "${ARCHIVE}" -type f | wc | awk '{print $1}'`
if [ "x$NUM_FILES" = "x" ] ; then
    echo "ERROR: unable to get a file count from $ARCHIVE"
    exit 1
fi

INST_SIZE=`du -k -s "${ARCHIVE}" | awk '{print $1}'`
if [ "x$INST_SIZE" = "x" ] ; then
    echo "ERROR: unable to get a usage size from $ARCHIVE"
    exit 1
fi

COMP_SIZE=`ls -l "${PKG_NAME}.pkg/Contents/Archive.pax.gz" | awk '{print $5}'`
COMP_SIZE=`echo "$COMP_SIZE 1024 / p" | dc`
if [ "x$COMP_SIZE" = "x" ] ; then
    echo "ERROR: unable to get the compressed archive size"
    exit 1
fi

cat > "${PKG_NAME}.pkg/Contents/Resources/${PKG_NAME}.sizes" <<EOF
NumFiles $NUM_FILES
InstalledSize $INST_SIZE
CompressedSize $COMP_SIZE
EOF
if [ ! -f "${PKG_NAME}.pkg/Contents/Resources/${PKG_NAME}.sizes" ] ; then
    echo "ERROR: unable to create the ${PKG_NAME}.size file"
    exit 1
fi

cat > "${PKG_NAME}.pkg/Contents/Resources/${PKG_NAME}.info" <<EOF
Title $NAME
Version $VERSION
Description $NAME $VERSION
DefaultLocation /
DeleteWarning Don't do it... untested.

### Package Flags

NeedsAuthorization NO
Required YES
Relocatable NO
RequiresReboot NO
UseUserMask YES
OverwritePermissions NO
InstallFat NO
RootVolumeOnly NO
EOF
if [ ! -f "${PKG_NAME}.pkg/Contents/Resources/${PKG_NAME}.info" ] ; then
    echo "ERROR: unable to create the ${PKG_NAME}.info file"
    exit 1
fi

cat > "${PKG_NAME}.pkg/Contents/Resources/Description.plist" <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple Computer//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
        <key>IFPkgDescriptionDeleteWarning</key>
        <string>Don't do it... untested.</string>
        <key>IFPkgDescriptionDescription</key>
        <string>${NAME} ${VERSION}</string>
        <key>IFPkgDescriptionTitle</key>
        <string>${NAME}</string>
        <key>IFPkgDescriptionVersion</key>
        <string>${VERSION}</string>
</dict>
</plist>
EOF
if [ ! -f "${PKG_NAME}.pkg/Contents/Resources/Description.plist" ] ; then
    echo "ERROR: unable to create the Description.plist file"
    exit 1
fi

ln -s ../Archive.bom "${PKG_NAME}.pkg/Contents/Resources/${PKG_NAME}.bom"
if [ $? != 0 ] ; then
    echo "ERROR: unable to successfully create a symbolic link to the Archive.bom"
    exit 1
fi
if [ ! -h "${PKG_NAME}.pkg/Contents/Resources/${PKG_NAME}.bom" ] ; then
    echo "ERROR: symbolic link ${PKG_NAME}.bom does not exist"
    exit 1
fi

ln -s ../Archive.pax.gz "${PKG_NAME}.pkg/Contents/Resources/${PKG_NAME}.pax.gz"
if [ $? != 0 ] ; then
    echo "ERROR: unable to successfully create a symbolic link to the Archive.pax.gz"
    exit 1
fi
if [ ! -h "${PKG_NAME}.pkg/Contents/Resources/${PKG_NAME}.pax.gz" ] ; then
    echo "ERROR: symbolic link ${PKG_NAME}.pax.gz does not exist"
    exit 1
fi

# woo hoo .. done

# Local Variables:
# mode: sh
# tab-width: 8
# sh-indentation: 4
# sh-basic-offset: 4
# indent-tabs-mode: t
# End:
# ex: shiftwidth=4 tabstop=8
