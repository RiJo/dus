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
source=("https://github.com/RiJo/$pkgname/archive/$pkgname-$pkgver.tar.gz")
md5sums=('3e86c45574e23da86cd4e9703f6838d1')

build() {
        cd "$srcdir/$pkgname-$pkgname-$pkgver"
        make
}

package() {
        cd "$srcdir/$pkgname-$pkgname-$pkgver"
        make DESTDIR="$pkgdir/" install
}
