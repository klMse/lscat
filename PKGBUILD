# Don't use this, it's bad
pkgname=lscat
pkgver=1.0.1
pkgrel=1
pkgdesc="Behaves like ls for directories or cat for files"
arch=('x86_64' 'i686')
license=('GPL')
optdepends=('coreutils')
makedepends=('make' 'gcc')
provides=('lscat')
source=('lscat.c' 'logging.c' 'logging.h' 'constants.h' 'Makefile' 'config')
sha256sums=('SKIP' 'SKIP' 'SKIP''SKIP' 'SKIP' 'SKIP')

build() {
    make lscat
}

package() {
    install -d "$pkgdir/usr/bin"
    install -d "$pkgdir/usr/share/doc/lscat/example"
    install -m 755 "$pkgname" "$pkgdir/usr/bin"
    install -m 644 config "$pkgdir/usr/share/doc/lscat/example"
}
