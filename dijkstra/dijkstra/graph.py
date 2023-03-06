
class Graph:
    def __init__(self):
        self.connections = dict()

    def add_single_conn(self, n1, n2):
        n1_conns = self.connections.get(n1, [])
        n1_conns.append(n2)
        self.connections[n1] = n1_conns

    def add_connection(self, n1, n2):
        self.add_single_conn(n1, n2)
        self.add_single_conn(n2, n1)
