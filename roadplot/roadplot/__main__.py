from dataclasses import dataclass

import osmium
import pygame.display


class MapLoaderHandler(osmium.SimpleHandler):
    included_ways = {'secondary', 'residential'}

    def __init__(self):
        super(MapLoaderHandler, self).__init__()
        self.stops = set()
        self.roads = set()
        self.nodes = dict()
        self.used_nodes = set()

    def node(self, n):
        self.nodes[n.id] = (n.location.lon, n.location.lat)

        if n.tags.get('public_transport', '') == 'stop_position':
            self.stops.add((n.location.lon, n.location.lat))

    def way(self, way):
        if not way.nodes or 'highway' not in way.tags or way.is_closed():
            return
        if way.tags['highway'] not in MapLoaderHandler.included_ways:
            return

        previous = way.nodes[0].ref
        self.used_nodes.add(previous)

        for i in range(1, len(way.nodes)):
            current = way.nodes[i].ref
            self.used_nodes.add(current)

            begin = self.nodes[previous]
            end = self.nodes[current]
            path = (begin, end)

            self.roads.add(path)

            previous = current

    def relation(self, way):
        """noop -> There's no need to handle relations, at least not now"""

    def drop_unused_nodes(self):
        self.nodes = {k: v for k, v in self.nodes.items() if k
                      in self.used_nodes}


def load_map(filename: str):
    handler = MapLoaderHandler()
    handler.apply_file(filename, locations=True)
    return handler


@dataclass
class Bounds:
    min_x: float
    min_y: float
    max_x: float
    max_y: float

    @property
    def min(self) -> tuple[float, float]:
        return (self.min_x, self.min_y)

    @property
    def max(self) -> tuple[float, float]:
        return (self.max_x, self.max_y)


def get_bounds(nodes: dict) -> Bounds:
    fst = next(iter(nodes.values()))
    bounds = Bounds(fst[0], fst[1], fst[0], fst[1])

    for node in nodes.values():
        x, y = node
        bounds.min_x = min(x, bounds.min_x)
        bounds.max_x = max(x, bounds.max_x)
        bounds.min_y = min(y, bounds.min_y)
        bounds.max_y = max(y, bounds.max_y)

    return bounds


def plot(handler: MapLoaderHandler):
    bounds = get_bounds(handler.nodes)
    ratio = (bounds.max_x - bounds.min_x) / (bounds.max_y - bounds.min_y)
    screen_size = (500, int(500*ratio))
    screen = pygame.display.set_mode(screen_size)
    print(screen_size)

    px = (bounds.max_x - bounds.min_x) / (screen_size[0] - 100)
    py = (bounds.max_y - bounds.min_y) / (screen_size[1] - 100*ratio)

    screen.fill((255, 255, 255))
    for stop in handler.stops:
        x, y = stop

        x = 50 + (x - bounds.min_x)/px
        y = screen_size[1] - 50*ratio - (y - bounds.min_y)/py
        pygame.draw.circle(screen, (255, 0, 0), (x, y), 5)

    for road in handler.roads:
        begin, end = road

        bx, by = begin
        bx = 50 + (bx - bounds.min_x)/px
        by = screen_size[1] - 50*ratio - (by - bounds.min_y)/py

        ex, ey = end
        ex = 50 + (ex - bounds.min_x)/px
        ey = screen_size[1] - 50*ratio - (ey - bounds.min_y)/py

        pygame.draw.line(screen, (0, 255, 0), (bx, by), (ex, ey), 3)

    pygame.display.flip()

    while True:
        for ev in pygame.event.get():
            if ev.type == pygame.QUIT:
                pygame.quit()
                return


def main():
    handler = load_map('poruba.osm')
    handler.drop_unused_nodes()
    print(f'{len(handler.stops)=}')
    print(f'{len(handler.roads)=}')
    print(f'{len(handler.nodes)=}')
    plot(handler)


if __name__ == '__main__':
    main()
