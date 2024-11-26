# Generated by hellwal

background='%%background%%'
foreground='%%foreground%%'
cursor='%%cursor%%'
border='%%border%%'

color0='%%color0.hex%%'
color1='%%color1.hex%%'
color2='%%color2.hex%%'
color3='%%color3.hex%%'
color4='%%color4.hex%%'
color5='%%color5.hex%%'
color6='%%color6.hex%%'
color7='%%color7.hex%%'
color8='%%color8.hex%%'
color9='%%color9.hex%%'
color10='%%color10.hex%%'
color11='%%color11.hex%%'
color12='%%color12.hex%%'
color13='%%color13.hex%%'
color14='%%color14.hex%%'
color15='%%color15.hex%%'

# FZF colors
export FZF_DEFAULT_OPTS="
    $FZF_DEFAULT_OPTS
    --color fg:7,bg:0,hl:1,fg+:232,bg+:1,hl+:255
    --color info:7,prompt:2,spinner:1,pointer:232,marker:1
"

# Fix LS_COLORS being unreadable.
export LS_COLORS="${LS_COLORS}:su=30;41:ow=30;42:st=30;44:"
