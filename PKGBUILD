# Maintainer: Rikard Johansson <rajoo@mail.com>

pkgname=dus
pkgver=0.0.9
pkgrel=1
pkgdesc="Console tool to summarize directory usage (improved GNU's `du -s`)"
arch=('any')
url="https://github.com/RiJo/dus"
license=('GPL3')
groups=()
depends=('glibc')
source=("https://github.com/RiJo/$pkgname/archive/v$pkgver.tar.gz")
md5sums=('beca11db0a3f6ae79ac3510cb0c3ab64')

build() {
        cd "$srcdir/$pkgname-$pkgver"
        make
}

package() {
        cd "$srcdir/$pkgname-$pkgver"
        make DESTDIR="$pkgdir/" install
}
