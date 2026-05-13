#!/bin/sh

PATH=/bin:/usr/bin:/usr/local/bin:/opt/homebrew/bin

if [ -z "$TEST_TMUX" ]; then
	TEST_TMUX=$(cd "$(dirname "$0")/.." && pwd)/tmux
fi
export TEST_TMUX

TMUX="$TEST_TMUX -Ltest"
TMUX_BAD="$TEST_TMUX -Ltest-bad"

$TMUX kill-server 2>/dev/null
$TMUX_BAD kill-server 2>/dev/null

CONF=$(mktemp)
OUT=$(mktemp)
BAD=$(mktemp)
SCRIPT_TEST=$(mktemp)
trap 'rm -f "$CONF" "$OUT" "$BAD" "$SCRIPT_TEST"; \
	$TMUX kill-server 2>/dev/null; $TMUX_BAD kill-server 2>/dev/null' \
	0 1 15

cat >"$CONF" <<EOF
set -g default-terminal "tmux-256color"
set -ga terminal-features "tmux-256color:Sync"
set -as terminal-features ",screen*:256:clipboard:ccolour:cstyle:focus:title"
set -g set-clipboard external
set -g focus-events on
EOF

run_pty() {
	_out=$1
	_cmd=$2

	if script -q "$SCRIPT_TEST" sh -c 'exit 0' >/dev/null 2>&1; then
		script -q "$_out" sh -c "$_cmd" >/dev/null 2>&1
	else
		script -q -c "$_cmd" "$_out" >/dev/null 2>&1
	fi
}

fail() {
	echo "$1" >&2
	exit 1
}

has_bytes() {
	_pattern=$(printf "$1")
	LC_ALL=C grep -Fq "$_pattern" "$OUT"
}

$TMUX -f"$CONF" new -d -s test -- sh -c '
	printf "\033[31mR\033[38;5;9mB\033[38;5;200mC\033[48;5;12mD\033[0m"
	while :; do sleep 1; done
' ||
	fail "failed to start tmux server"

run_pty "$OUT" \
	'TERM=tmux-256color "$TEST_TMUX" -Ltest -f/dev/null attach -t test' &
PTY_PID=$!

sleep 6

$TMUX has 2>/dev/null || fail "server exited during tmux-256color attach"

CLIENT=$($TMUX list-clients -F '#{client_termname} #{client_termfeatures}')
case "$CLIENT" in
	*tmux-256color*bpaste*focus*RGB*sync*title*) ;;
	*) fail "unexpected client terminal features: $CLIENT" ;;
esac

[ "$($TMUX show -gv default-terminal)" = "tmux-256color" ] ||
	fail "default-terminal was not tmux-256color"
[ "$($TMUX show -gv set-clipboard)" = "external" ] ||
	fail "set-clipboard was not external"
[ "$($TMUX show -gv focus-events)" = "on" ] ||
	fail "focus-events was not on"

FEATURES=$($TMUX show -gv terminal-features)
case "$FEATURES" in
	*tmux-256color:Sync*screen\*:256:clipboard:ccolour:cstyle:focus:title*) ;;
	*) fail "terminal-features did not preserve required entries" ;;
esac

$TMUX kill-server 2>/dev/null
wait "$PTY_PID" 2>/dev/null

if grep -q 'server exited unexpectedly\|unsupported terminal' "$OUT"; then
	fail "tmux-256color attach failed"
fi
has_bytes '\033[?1049h' || fail "smcup bytes were not emitted"
has_bytes '\033[H\033[J' || fail "clear bytes were not emitted"
has_bytes '\033[31m' || fail "setaf low-colour bytes were not emitted"
has_bytes '\033[91m' || fail "setaf bright-colour bytes were not emitted"
has_bytes '\033[38;5;200m' || fail "setaf 256-colour bytes were not emitted"
has_bytes '\033[104m' || fail "setab bright-colour bytes were not emitted"
has_bytes '\033[?2026h' || fail "Sync enable bytes were not emitted"
has_bytes '\033[?2026l' || fail "Sync disable bytes were not emitted"
has_bytes '\033[?1004h' || fail "focus event enable bytes were not emitted"

if run_pty "$BAD" \
	'TERM=ansi "$TEST_TMUX" -Ltest-bad -f/dev/null new -- sh -c "while :; do sleep 1; done"'
then
	fail "unsupported TERM was accepted"
fi
if ! grep -q 'unsupported terminal: ansi' "$BAD"; then
	fail "unsupported TERM did not report a useful error"
fi

exit 0
