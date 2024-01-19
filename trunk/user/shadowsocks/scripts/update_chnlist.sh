#!/bin/sh

set -e -o pipefail

[ "$1" != "force" ] && [ "$(nvram get ss_update_chnlist)" != "1" ] && exit 0
CHNLIST_URL="$(nvram get chnlist_url)"

logger -st "SSP[$$]Update" "开始更新白名单..."

rm -f /tmp/chnlist_domain.txt
curl -k -s --connect-timeout 5 --retry 3 \
${CHNLIST_URL:-"https://raw.githubusercontent.com/felixonmars/dnsmasq-china-list/master/accelerated-domains.china.conf"} | \
awk -F/ '{print $2}' | sort -u > /tmp/chnlist_domain.txt

[ ! -d /etc/storage/chnlist/ ] && mkdir /etc/storage/chnlist/
[ -s /tmp/chnlist_domain.txt ] && mv -f /tmp/chnlist_domain.txt /etc/storage/chnlist/chnlist_domain.txt && \
mtd_storage.sh save >/dev/null 2>&1

[ "$(nvram get ss_enable)" == "1" ] && /usr/bin/shadowsocks.sh rednsconf &>/dev/null

logger -st "SSP[$$]Update" "白名单更新完成"

