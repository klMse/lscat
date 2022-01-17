pkgname=lscat
pkgver=1.0.1
pkgrel=1
pkgdesc="Cat and ls in one"
arch=('i686' 'x86_64')
license=('GPL')
depends=('coreutils')
makedepends=('make' 'gcc')
provides=('lscat')
source=('lscat.c' 'logging.c' 'logging.h' 'constants.h' 'Makefile' 'config')
sha256sums=('SKIP' 'SKIP' 'SKIP' 'SKIP' 'SKIP' 'SKIP')

build() {
    make lscat
}

package() {
    install -d "$pkgdir/usr/bin"
    install -d "$pkgdir/usr/share/lscat"
    install -m 755 "$pkgname" "$pkgdir/usr/bin"
    install -m 644 config "$pkgdir/usr/share/lscat"
}
