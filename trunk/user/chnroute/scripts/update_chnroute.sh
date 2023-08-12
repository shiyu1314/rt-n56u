#!/bin/sh

set -e -o pipefail

[ "$1" != "force" ] && [ "$(nvram get ss_update_chnroute)" != "1" ] && exit 0
USBB_DIR=$(find /media/ -name SSP)
CONF_DIR="/tmp/SSP"
[ -n "$USBB_DIR" ] && EXTB_DIR="$USBB_DIR" || EXTB_DIR="$CONF_DIR"
CHNROUTE_URL="$(nvram get chnroute_url)"
APNIC_URL="http://ftp.apnic.net/apnic/stats/apnic/delegated-apnic-latest"
user_agent="Mozilla/5.0 (X11; Linux; rv:74.0) Gecko/20100101 Firefox/74.0"

(logger -st "SSP[$$]Update" "开始更新路由表..."
[ -e $CONF_DIR/chnroute.txt ] && rm -rf $CONF_DIR/chnroute.txt
[ -e $EXTB_DIR/chnroute.old ] && rm -rf $EXTB_DIR/chnroute.old
[ -e $EXTB_DIR/chnroute.txt ] && mv -f $EXTB_DIR/chnroute.txt $EXTB_DIR/chnroute.old
if [ -z "$CHNROUTE_URL" ]; then
	curl -k -s -A "$user_agent" --connect-timeout 5 --retry 3 "$APNIC_URL" | sed '/\*/d' | \
	awk -F\| '/CN\|ipv4/ { printf("%s/%d\n", $4, 32-log($5)/log(2)) }' > $EXTB_DIR/chnroute.txt
else
	curl -k -s --connect-timeout 5 --retry 3 -o $EXTB_DIR/chnroute.txt "$CHNROUTE_URL" && \
	sed -i '/^#/d' $EXTB_DIR/chnroute.txt && sed -i '/^$/d' $EXTB_DIR/chnroute.txt
fi)&

wait

if [ ! -e $EXTB_DIR/chnroute.txt ]; then
	[ -e $EXTB_DIR/chnroute.old ] && mv -f $EXTB_DIR/chnroute.old $EXTB_DIR/chnroute.txt
	logger -st "SSP[$$]Update" "路由表更新失败" && exit 0
fi
[ ! -d /etc/storage/chinadns/ ] && mkdir /etc/storage/chinadns/
if [ $(cat $EXTB_DIR/chnroute.txt | wc -l) -le 65536 ]; then
	mv -f $EXTB_DIR/chnroute.txt /etc/storage/chinadns/chnroute.txt
	mtd_storage.sh save >/dev/null 2>&1
	logger -st "SSP[$$]Update" "保存到内部存储"
else
	rm -rf /etc/storage/chinadns/chnroute.txt && \
	ln -sf $EXTB_DIR/chnroute.txt /etc/storage/chinadns/chnroute.txt
	[ "$EXTB_DIR" = "$CONF_DIR" ] && logger -st "SSP[$$]Update" "保留在临时目录"
	[ "$EXTB_DIR" = "$USBB_DIR" ] && logger -st "SSP[$$]Update" "保存到外部存储"
fi
[ "$(nvram get ss_enable)" = "1" ] && echo "1" > $CONF_DIR/startrules && \
/usr/bin/shadowsocks.sh restart &>/dev/null

logger -st "SSP[$$]Update" "路由表更新完成"
