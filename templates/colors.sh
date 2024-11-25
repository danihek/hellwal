# Generated by hellwal

background='%%color|0|.hex%%'
foreground='%%color|15|.hex%%'

color0='%%color|0|.hex%%'
color1='%%color|1|.hex%%'
color2='%%color|2|.hex%%'
color3='%%color|3|.hex%%'
color4='%%color|4|.hex%%'
color5='%%color|5|.hex%%'
color6='%%color|6|.hex%%'
color7='%%color|7|.hex%%'
color8='%%color|8|.hex%%'
color9='%%color|9|.hex%%'
color10='%%color|10|.hex%%'
color11='%%color|11|.hex%%'
color12='%%color|12|.hex%%'
color13='%%color|13|.hex%%'
color14='%%color|14|.hex%%'
color15='%%color|15|.hex%%'

# FZF colors
export FZF_DEFAULT_OPTS="
    $FZF_DEFAULT_OPTS
    --color fg:7,bg:0,hl:1,fg+:232,bg+:1,hl+:255
    --color info:7,prompt:2,spinner:1,pointer:232,marker:1
"

# Fix LS_COLORS being unreadable.
export LS_COLORS="${LS_COLORS}:su=30;41:ow=30;42:st=30;44:"
