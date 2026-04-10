# Branch: optimized-scrolling

Modifications from upstream tmux on this branch. Targeted at macOS with
`default-terminal "tmux-256color"` and `terminal-features "tmux-256color:Sync"`.

## No autotools (`./configure`)

`configure` is a hand-written shell script that replaces the autotools
`configure.ac`/`Makefile.am` stack. It probes only what is actually needed on
macOS, generates `config.h` and `build.ninja`, and completes in under a second.

Usage:
```
./configure [--enable-debug] [--enable-utf8proc] [--enable-sixel] [--prefix=DIR]
ninja
```

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

## Optimized copy-mode scrolling

Several changes to `window-copy.c` and `server-client.c` that make scrolling
through large buffers much faster.

### Bug fix: `window_copy_write_lines` wrong loop variable

`window_copy_write_lines` iterated with `yy` but passed `py` (which never
changed) to `window_copy_write_line`, so only the first line was redrawn on
every iteration. Fixed to pass `yy`.

### Format tree hoisted out of per-line loop

`window_copy_write_line` created a format tree and applied four styles on every
call. `window_copy_write_lines` called it in a loop. The format tree is now
created once for the whole batch and passed in, eliminating redundant allocation
and string evaluation per line.

### `insertline`/`deleteline` for page up/down

When a page-up or page-down in copy mode results in a pure scroll (cursor stays
on screen, only `oy` changes), the existing terminal content is shifted with
`screen_write_insertline`/`deleteline` and only the newly revealed strip is
redrawn. Previously the entire screen was redrawn.

### Timer-based wheel-event coalescing

Consecutive wheel events accumulate a scroll delta rather than rendering
immediately. A 16 ms timer (~60 fps cap) fires the actual render. The deferred
path skips search-mark updates, selection updates, and format trees, and uses
`insertline`/`deleteline` to shift existing content while only drawing newly
exposed lines by copying raw cells from the backing grid. If the client output
buffer is backed up, rendering is deferred further at 1 ms retry intervals.
Any non-scroll command flushes pending scroll delta before executing.

### Short-circuit wheel events before the command queue

Wheel events targeting a pane in copy or view mode are intercepted in
`server_client_handle_key` before a `cmdq` item is allocated. This bypasses
`cmdq` allocation/dispatch, `server_client_check_mouse`, `cmd_find_from_mouse`,
and key-binding lookup, going directly to `window_copy_scroll_accumulate`.

### Hide position indicator by default in copy mode

`copy-mode-position` defaults to `off` instead of `on`.
