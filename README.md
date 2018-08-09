# Katarakt

katarakt is a simple PDF viewer. It is designed to use as much available screen space as possible.

### Features

There are currently three layouts.

* The single layout is active by default. As the name suggests it shows one slide at a time. Each page individually is scaled to use the maximum available screen space.
* The grid layout is a continuous viewing mode, and it supports as many columns as you want. The relative size between pages is preserved, and the biggest page is zoomed to fit the width of the window.
* The presenter layout shows the current and the next slide side by side and opens another window to view on the projector.

In addition, with katarakt you can

* search for text and step through the results
* rotate pages in steps of 90 degrees
* click on links to navigate through the document
* reload the file to see changes e. g. while editing a LaTeX document. With inotify, the document will automatically be reloaded when it changes.
* set your own keybindings and customise options.
* open a URL. Remote documents are downloaded automatically.

Input is always responsive, because CPU-intensive operations like rendering and searching are done in parallel. Also, you don’t have to wait after changing the scale or rotation, because the available pages are used until they get seamlessly replaced by new ones.

### Dependencies

* `qt` version 4 or 5
* `poppler-qt` 4 or 5 ≥  0.18

### Source code


You can browse the [git repo](https://gitlab.cs.fau.de/Qui_Sum/katarakt) online or clone it directly:

```
git clone https://gitlab.cs.fau.de/Qui_Sum/katarakt.git
```

Tarballs are available [here](https://wwwcip.cs.fau.de/~go18gomu/katarakt/tarballs/).

### Community

Feel free to join `#katarakt` on `freenode`.

### License

Copyright © 2011-2018, Philipp Erhardt. All rights reserved.

katarakt is free software. You can redistribute it and/or modify it under the terms of the simplified BSD license.
