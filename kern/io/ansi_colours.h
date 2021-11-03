

#pragma once

#define ANSI_START      "\x1b["
#define ANSI_END        "m"

#ifndef CONFIG_DISABLE_ANSI
#define ANSI_CODE(x)    ANSI_START #x ANSI_END
#else
#define ANSI_CODE(x)    ""
#endif

#define ANSI_COLOR_RESET            ANSI_CODE(0)

#define ANSI_COLOR_BLACK            ANSI_CODE(30)
#define ANSI_COLOR_RED              ANSI_CODE(31)
#define ANSI_COLOR_GREEN            ANSI_CODE(32)
#define ANSI_COLOR_YELLOW           ANSI_CODE(33)
#define ANSI_COLOR_BLUE             ANSI_CODE(34)
#define ANSI_COLOR_MAGENTA          ANSI_CODE(35)
#define ANSI_COLOR_CYAN             ANSI_CODE(36)
#define ANSI_COLOR_WHITE            ANSI_CODE(37)
#define ANSI_COLOR_GREY             ANSI_CODE(90)
#define ANSI_COLOR_BRIGHT_RED       ANSI_CODE(91)
#define ANSI_COLOR_BRIGHT_GREEN     ANSI_CODE(92)
#define ANSI_COLOR_BRIGHT_YELLOW    ANSI_CODE(93)
#define ANSI_COLOR_BRIGHT_BLUE      ANSI_CODE(94)
#define ANSI_COLOR_BRIGHT_MAGENTA   ANSI_CODE(95)
#define ANSI_COLOR_BRIGHT_CYAN      ANSI_CODE(96)
#define ANSI_COLOR_BRIGHT_WHITE     ANSI_CODE(97)

