;;; hellwal-theme.el --- Theme using Hellwal variables

(deftheme hellwal "Theme using Hellwal variables.")

(let* (
      ;; Hellwal semantic colors
      (bg "#%%background.hex%%")
      (on-background "#%%foreground.hex%%")

      ;; Assign Hellwal color slots
      (primary "#%%color4.hex%%")
      (primary-container "#%%color12.hex%%")
      (on-primary "#%%color15.hex%%")
      (on-primary-container "#%%background.hex%%")

      (secondary "#%%color3.hex%%")
      (secondary-container "#%%color11.hex%%")
      (on-secondary "#%%color15.hex%%")
      (on-secondary-container "#%%background.hex%%")

      (tertiary "#%%color5.hex%%")
      (tertiary-container "#%%color13.hex%%")
      (on-tertiary "#%%color15.hex%%")
      (on-tertiary-container "#%%background.hex%%")

      (err "#%%color1.hex%%")
      (err-container "#%%color9.hex%%")
      (on-err "#%%background.hex%%")
      (on-err-container "#%%foreground.hex%%")

      (outline-color "#%%color7.hex%%")
      (outline-variant "#%%color8.hex%%")

      (surface "#%%background.hex%%")
      (surface-container "#%%color0.hex%%")
      (surface-container-low "#%%color10.hex%%")
      (surface-container-high "#%%color14.hex%%")
      (surface-container-highest "#%%color15.hex%%")
      (surface-container-lowest "#%%color8.hex%%")

      (surface-variant "#%%color7.hex%%")

      ;; Success mapped to green-ish Hellwal slots
      (success "#%%color2.hex%%")
      (success-container "#%%color10.hex%%")
      (on-success "#%%color15.hex%%")
      (on-success-container "#%%background.hex%%")

      ;; Term colors
      (term0 "#%%color0.hex%%")
      (term1 "#%%color1.hex%%")
      (term2 "#%%color2.hex%%")
      (term3 "#%%color3.hex%%")
      (term4 "#%%color4.hex%%")
      (term5 "#%%color5.hex%%")
      (term6 "#%%color6.hex%%")
      (term7 "#%%color7.hex%%")
      (term8 "#%%color8.hex%%")
      (term9 "#%%color9.hex%%")
      (term10 "#%%color10.hex%%")
      (term11 "#%%color11.hex%%")
      (term12 "#%%color12.hex%%")
      (term13 "#%%color13.hex%%")
      (term14 "#%%color14.hex%%")
      (term15 "#%%color15.hex%%")
)

  (custom-theme-set-faces
   'hellwal

   ;; BASIC
   `(default ((t (:background ,bg :foreground ,on-background))))
   `(cursor ((t (:background ,primary))))

   ;; Highlights
   `(highlight ((t (:background ,primary-container :foreground ,on-primary-container))))
   `(region ((t (:background ,primary-container :foreground ,on-primary-container :extend t))))
   `(secondary-selection ((t (:background ,secondary-container :foreground ,on-secondary-container :extend t))))

   ;; Search
   `(isearch ((t (:background ,tertiary-container :foreground ,on-tertiary-container :weight bold))))
   `(lazy-highlight ((t (:background ,secondary-container :foreground ,on-secondary-container))))

   ;; Borders, fringe
   `(vertical-border ((t (:foreground ,surface-variant))))
   `(border ((t (:background ,surface-variant :foreground ,surface-variant))))
   `(fringe ((t (:background ,surface :foreground ,outline-variant))))
   `(shadow ((t (:foreground ,outline-variant))))

   ;; Links, statuses
   `(link ((t (:foreground ,primary :underline t))))
   `(link-visited ((t (:foreground ,tertiary :underline t))))
   `(success ((t (:foreground ,success))))
   `(warning ((t (:foreground ,secondary))))
   `(error ((t (:foreground ,err))))

   ;; Font-lock
   `(font-lock-builtin-face ((t (:foreground ,primary))))
   `(font-lock-comment-face ((t (:foreground ,outline-color :slant italic))))
   `(font-lock-comment-delimiter-face ((t (:foreground ,outline-variant))))
   `(font-lock-constant-face ((t (:foreground ,tertiary :weight bold))))
   `(font-lock-doc-face ((t (:foreground ,surface-variant :slant italic))))
   `(font-lock-function-name-face ((t (:foreground ,primary :weight bold))))
   `(font-lock-keyword-face ((t (:foreground ,secondary :weight bold))))
   `(font-lock-string-face ((t (:foreground ,tertiary))))
   `(font-lock-type-face ((t (:foreground ,primary-container))))
   `(font-lock-variable-name-face ((t (:foreground ,on-background))))
   `(font-lock-warning-face ((t (:foreground ,err :weight bold))))

   ;; Parens
   `(show-paren-match ((t (:background ,primary-container :foreground ,on-primary-container :weight bold))))
   `(show-paren-mismatch ((t (:background ,err-container :foreground ,on-err-container :weight bold))))

   ;; MODELINE
   `(mode-line ((t (:background ,surface-container :foreground ,on-background :box nil))))
   `(mode-line-inactive ((t (:background ,surface :foreground ,surface-variant :box nil))))
   `(mode-line-buffer-id ((t (:foreground ,primary :weight bold))))

   ;; ORG MODE
   `(org-block ((t (:background ,surface-container-low :extend t :inherit fixed-pitch))))
   `(org-block-begin-line ((t (:background ,surface-container-low :foreground ,primary-container :extend t :slant italic :inherit fixed-pitch))))
   `(org-block-end-line ((t (:background ,surface-container-low :foreground ,primary-container :extend t :slant italic :inherit fixed-pitch))))
   `(org-code ((t (:background ,surface-container-low :foreground ,tertiary :inherit fixed-pitch))))
   `(org-verbatim ((t (:background ,surface-container-low :foreground ,primary :inherit fixed-pitch))))
   `(org-level-1 ((t (:foreground ,primary :weight bold :height 1.2))))
   `(org-level-2 ((t (:foreground ,primary-container :weight bold :height 1.1))))
   `(org-level-3 ((t (:foreground ,secondary :weight bold))))
   `(org-level-4 ((t (:foreground ,secondary-container :weight bold))))
))

(with-eval-after-load 'org
  (setq org-hide-leading-stars t)
  (setq org-startup-indented t))

(provide-theme 'hellwal)
