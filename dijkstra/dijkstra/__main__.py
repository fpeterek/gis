from dataclasses import dataclass

import osmium
import pygame.display
import geopy.distance

from graph import Graph


class MapLoaderHandler(osmium.SimpleHandler):
    included_ways = {'secondary', 'residential', 'primary'}

    def __init__(self):
        super(MapLoaderHandler, self).__init__()
        self.roads = set()
        self.nodes = dict()
        self.used_nodes = set()
        self.graph = Graph()

    def node(self, n):
        self.nodes[n.id] = (n.location.lon, n.location.lat)

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
            self.graph.add_connection(begin, end)

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


def distance(fst, snd):
    return geopy.distance.distance(fst[::-1], snd[::-1]).meters


def find_nearest_node(point, nodes):
    nearest = None
    min_dist = 2**64
    for node in nodes:
        dist = distance(point, node)
        if dist < min_dist:
            nearest = node
            min_dist = dist
    return nearest


def next_node(distances, processed):
    min_dist = 2**63
    min_node = None
    for node, dist in distances.items():
        if dist < min_dist and node not in processed:
            min_node = node
            min_dist = dist

    return min_node


def dijkstra(
        begin: tuple[float, float],
        end: tuple[float, float],
        graph: Graph):
    shortest_paths = dict()
    dists = {begin: 0}
    unprocessed = set(graph.connections.keys())
    processed = set()

    while unprocessed:
        node = next_node(dists, processed)

        if node is None or node == end:
            break

        unprocessed.remove(node)
        processed.add(node)

        for conn in graph.connections[node]:
            if conn in processed:
                continue
            conn_dist = dists[node] + distance(conn, node)
            if conn_dist < dists.get(conn, 2**63):
                dists[conn] = conn_dist
                shortest_paths[conn] = node

    solution = [end]
    start = shortest_paths[end]

    while start != begin:
        solution.append(start)
        start = shortest_paths[start]

    solution.append(start)
    return solution[::-1]


def plot(handler: MapLoaderHandler):
    bounds = get_bounds(handler.nodes)
    ratio = (bounds.max_x - bounds.min_x) / (bounds.max_y - bounds.min_y)
    screen_size = (500, int(500*ratio))
    screen = pygame.display.set_mode(screen_size)
    print(screen_size)

    px = (bounds.max_x - bounds.min_x) / (screen_size[0] - 100)
    py = (bounds.max_y - bounds.min_y) / (screen_size[1] - 100*ratio)

    screen.fill((255, 255, 255))

    begin = (49.8305753, 18.1601472)[::-1]
    end = (49.8216433, 18.1941897)[::-1]

    begin = find_nearest_node(begin, handler.nodes.values())
    end = find_nearest_node(end, handler.nodes.values())

    path = dijkstra(begin, end, handler.graph)
    print(path)

    for point in (begin, end):
        x, y = point

        x = 50 + (x - bounds.min_x)/px
        y = screen_size[1] - 50*ratio - (y - bounds.min_y)/py
        print(point, x, y)
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

    for begin, end in zip(path[:-1], path[1:]):

        bx, by = begin
        bx = 50 + (bx - bounds.min_x)/px
        by = screen_size[1] - 50*ratio - (by - bounds.min_y)/py

        ex, ey = end
        ex = 50 + (ex - bounds.min_x)/px
        ey = screen_size[1] - 50*ratio - (ey - bounds.min_y)/py

        pygame.draw.line(screen, (0, 0, 255), (bx, by), (ex, ey), 3)

    pygame.display.flip()

    while True:
        for ev in pygame.event.get():
            if ev.type == pygame.QUIT:
                pygame.quit()
                return


def main():
    handler = load_map('poruba.osm')
    handler.drop_unused_nodes()
    print(f'{len(handler.roads)=}')
    print(f'{len(handler.nodes)=}')
    plot(handler)


if __name__ == '__main__':
    main()
