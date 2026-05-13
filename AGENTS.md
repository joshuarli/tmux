tmux with the following modifications:
- no ncurses
- no autotools (./configure, ninja)

## No autotools (`./configure`)

`configure` is a hand-written shell script that replaces the autotools
`configure.ac`/`Makefile.am` stack. It probes only what is actually needed on
macOS and Linux, generates `config.h` and `build.ninja`, and completes in under a second.

Usage:
```
./configure [--enable-debug] [--prefix=DIR]
ninja
```

`utf8proc` is required. Sixel support remains available through upstream
autotools but is intentionally omitted from the hand-written `configure`.

The autotools files (`configure.ac`, `Makefile.am`, etc.) are still present for
upstream compatibility but are not used by this workflow.


## No ncurses

tmux's only ncurses dependency was the terminfo query API: `setupterm`,
`tigetstr/tigetnum/tigetflag`, `tparm/tiparm`, and `del_curterm`. No curses
screen management was ever used.

This branch replaces that dependency entirely:

- **Hardcoded `tmux-256color` capability table** in `tty-term.c`. The table
  contains the same binary escape sequences that `tigetstr` would have returned.
  Only capabilities present in tmux's internal `tty_term_codes` table are
  included. Attempting to attach with any other `$TERM` produces an explicit
  error.

- **`tmux_tparm()`** — a ~120-line stack-based interpreter of the terminfo
  parameter language, also in `tty-term.c`. Handles the full spec needed for
  `tmux-256color`: push/pop (`%p1`–`%p9`, `%{n}`, `%'c'`), arithmetic
  (`%+`, `%-`, `%*`, `%/`), bitwise (`%&`, `%|`, `%^`), comparisons
  (`%<`, `%>`, `%=`), logical (`%A`, `%O`, `%!`), `%i` (increment first two
  params), `%d`/`%c`/`%s` output, and full `%?`/`%t`/`%e`/`%;` conditionals
  including chained else-if without a second `%?` (as used by `setaf`/`setab`).

- `configure` no longer detects or links ncurses. `NCURSES_CFLAGS`,
  `NCURSES_LIBS`, and the `tiparm_s` probe are gone.

`terminal-features "tmux-256color:Sync"` continues to work — features are
applied via tmux's override system, independent of the terminfo lookup.

`regress/no-ncurses.sh` covers the required no-ncurses configuration:
```
set -g default-terminal "tmux-256color"
set -ga terminal-features "tmux-256color:Sync"
set -as terminal-features ",screen*:256:clipboard:ccolour:cstyle:focus:title"
set -g set-clipboard external
set -g focus-events on
```
