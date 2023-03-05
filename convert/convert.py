import pyproj


class GeoConverter:
    def __init__(self):
        self.sjtsk = pyproj.Proj(
                's-jtsk: + proj = krovak + ellps = bessel + ' +
                'towgs84 = 570.8,85.7,462.8,4.998,1.587,5.261,3.56'
                )
        self.wgs84 = pyproj.Proj(
                'wgs84: + proj = longlat + ellps = WGS84 + datum ' +
                '= WGS84 + no_defs')

    def convert(self, x, y) -> tuple[float, float]:
        lon, lat = pyproj.transform(self.sjtsk, self.wgs84, x, y)
        return lat, lon


if __name__ == '__main__':
    converter = GeoConverter()
    lat, lon = converter.convert(-453462.187500, -1101354.750000)
    print(lat, lon)
    lat, lon = converter.convert(-452927.250000, -1100794.750000)
    print(lat, lon)
