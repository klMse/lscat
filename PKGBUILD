pkgname=lscat
pkgver=1.0.0
pkgrel=1
pkgdesc="Cat and ls in one"
arch=('i686' 'x86_64')
license=('GPL')
depends=('coreutils')
makedepends=('make')
provides=('lscat')
source=('lscat.c' 'Makefile')
sha256sums=('SKIP' 'SKIP')

build() {
    make lscat
}

package() {
    install -d "$pkgdir/usr/bin"
    install -m 755 "$pkgname" "$pkgdir/usr/bin"
}
