#!/bin/sh

rootdir="$1"
[ -n "$rootdir" ] || rootdir=.
if [ -z "$PACKAGE_VERSION" ]; then
	eval "$(cd "$rootdir" && grep -m1 PACKAGE_VERSION configure \
		2>/dev/null)"
fi

[ ! -f "$1/VERSION" ] || VN="$(cat VERSION)"

if [ -z "$VN" ] && [ -d "$rootdir/.git" ]; then
	VN=$(cd "$rootdir" &&
		git describe --tags --match 'v[0-9]*' 2>/dev/null)

	if [ -z "$VN" ]; then
		VN=$(cd "$rootdir" &&
			git log -1 --pretty=format:"git-%cd-%h" --date=short \
			2>/dev/null)
		[ -z "$VN" ] || VN="$PACKAGE_VERSION-$VN"
	fi
fi

# try substituted hash if any
if [ -z "$VN" ]; then
	git_date="$Format:%ci"
	git_hash="$Format:%h$"
	if ! [ "${git_hash:0:1}" = "\$" ]; then
		git_date="${git_date%% *}"
		VN="$PACKAGE_VERSION-$git_date-$git_hash"
	fi
fi

# try name of directory (from github tarballs)
if [ -z "$VN" ]; then
	srcdir="$(cd "$1" && pwd)"
	case "$srcdir" in
	  */*-*-[0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f])
		git_hash="${srcdir##*-}"
		[ -z "$PACKAGE_VERSION" ] || VN="$PACKAGE_VERSION-$git_hash"
		;;
	esac
fi

VN=$(expr "$VN" : v*'\(.*\)')
[ -n "$VN" ] || VN="$PACKAGE_VERSION"

if [ -n "$VN" ]; then
	echo "$VN"
else
	echo "0-UNKNOWN"
fi
