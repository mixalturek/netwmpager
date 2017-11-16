# netwmpager

EWMH (NetWM) compatible pager

Copyright Timo Hirvonen

Netwmpager is a tiny NetWM / EWMH compatible pager with minimal dependencies. It should work with all NetWM compatible window managers, tested with Openbox and Pekwm. Transparency requires XComposite support.

![Screenshot](https://raw.githubusercontent.com/mixalturek/netwmpager/master/doc/netwmpager-transparent.png)
![Screenshot](https://raw.githubusercontent.com/mixalturek/netwmpager/master/doc/netwmpager.png)


## Installation

### Manual compilation

Download a tarbal from Releases/Tags.

```shell
tar -xf netwmpager-1.11.2.tar.gz
cd netwmpager-1.11.2

./configure --help
./configure
make

sudo make install

mkdir -p ~/.config/netwmpager
cp config-example ~/.config/netwmpager/config
```

### Debian package

Download a tarbal from Releases/Tags.

```shell
cp netwmpager-1.11.2.tar.gz netwmpager_1.11.2.orig.tar.gz
tar -xf netwmpager_1.11.2.orig.tar.gz
cd netwmpager-1.11.2

debuild -us -uc

sudo dpkg -i ../netwmpager_1.11.2-*.deb

mkdir -p ~/.config/netwmpager
cp /usr/share/netwmpager/config-example ~/.config/netwmpager/config
```


## Usage

See `netwmpager -help` for instructions.

```shell
netwmpager &
```


## ChangeLog

- 1.11 (about 2006-04-25)
    - ?
- 1.10 (2006-04-15)
    - Small bug fix.
- 1.9 (2006-04-07)
    - Made automatic height calculation optional and fixed some bugs.
- 1.8 (2006-04-05)
    - Bug fixes.
- 1.7 (2005-12-18)
    - Set WM_CLASS property for the pager window.
    - This should make it possible to hide netwmpager from pypanel.


## Authors

Timo Hirvonen (tihirvon AT gmail.com)


## Contributors

Marc Wilson (msw AT cox.net)
- Aspect ratio fix.
 
Michal Turek (mixalturek AT users.sf.net)
- Patch for incorrect desktop switching applied.
- Debian package.
- Manual page.


## Notes About This Repository

There is a new netwmpager 2.x for download at [http://sourceforge.net/projects/sf-xpaint/files/netwmpager/](http://sourceforge.net/projects/sf-xpaint/files/netwmpager/) But this version doesn't work for me as good as older 1.11 and I don't have enough patience for investigation why. I tried a lot with the config file but I wasn't successful in several hours.

The original Timo's netwmpager website [http://onion.dynserv.net/~timo/netwmpager.html](http://onion.dynserv.net/~timo/netwmpager.html) doesn't exist any more (still available at [http://web.archive.org/](http://web.archive.org/)) and the 1.11 tarbal is disappearing from the web, so I decided to backup the code publicly on GitHub. I had its copy on my disk and I found [https://github.com/Youx/netwmpager](https://github.com/Youx/netwmpager) with the same content. The license is GPL so everything should be OK. Netwmpager 1.11 already do everything I need so I plan no big improvements of the code at the moment.

[https://github.com/mixalturek/netwmpager](https://github.com/mixalturek/netwmpager)
