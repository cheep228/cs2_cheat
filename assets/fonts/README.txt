The supplied Neverlose source references these fonts but does not include them:

museosanscyrl-500.ttf
museosanscyrl-700.ttf
museosanscyrl-900.ttf
neverlose-v2-icons.ttf

If you legally have those files, place them in this directory OR in a "fonts"
folder next to cs2_overlay.dll. The UI loader also checks C:\NEVERLOSE\Fonts\.

Without them the build uses Windows Segoe UI/Arial fallbacks at the exact source
sizes. Sidebar icon glyphs are hidden until neverlose-v2-icons.ttf is present.
