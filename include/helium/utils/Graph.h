#ifndef GRAPH_H
#define GRAPH_H

#include <boost/utility.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/topological_sort.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/graph/visitors.hpp>

// using namespace boost;


namespace hebigraph {

  using namespace boost;
  // using namespace std;
  // typedef pair<int,int> Edge;

  struct edge_label_t {
    typedef edge_property_tag kind;
  };
  typedef property<edge_label_t, std::string> EdgeLabelProperty;

  typedef adjacency_list<vecS, vecS, bidirectionalS, no_property, EdgeLabelProperty> GraphType;
  
  typedef graph_traits<GraphType>::vertex_descriptor Vertex;
  typedef graph_traits<GraphType>::edge_descriptor Edge;
  typedef graph_traits<GraphType>::vertex_iterator VertexIter;
  typedef graph_traits<GraphType>::edge_iterator EdgeIter;


  struct cycle_detector : public dfs_visitor<>
  {
    cycle_detector(bool& has_cycle) 
      : m_has_cycle(has_cycle) { }

    template <class Edge, class Graph>
    void back_edge(Edge, Graph&) { m_has_cycle = true; }
  protected:
    bool& m_has_cycle;
  };

  
  template <typename T> class Graph {
  public:
    Graph() {}
    ~Graph() {}
    
    bool hasEdge(T x, T y) {
      return edge(Node2Vertex[x], Node2Vertex[y], g).second;
    }
    void addEdge(T x, T y, std::string label="") {
      if (hasEdge(x, y)) return;
      Edge e = add_edge(Node2Vertex[x], Node2Vertex[y], g).first;
      // add edge label
      put(edge_label_t(), g, e, label);
    }
    void removeEdge(T x, T y) {
      if (!hasEdge(x,y)) return;
      remove_edge(Node2Vertex[x], Node2Vertex[y], g);
    }
    void addEdge(std::set<T> fromset, std::set<T> toset, std::string label="") {
      for (T from : fromset) {
        for (T to : toset) {
          addEdge(from, to, label);
        }
      }
    }
    void addEdge(std::set<T> fromset, T to, std::string label="") {
      for (T from : fromset) {
        addEdge(from, to, label);
      }
    }
    void addEdge(T from, std::set<T> toset, std::string label="") {
      for (T to : toset) {
        addEdge(from, to, label);
      }
    }

    bool hasNode(T x) {
      return Node2Vertex.count(x) == 1;
    }
    void addNode(T x) {
      if (!hasNode(x)) {
        Vertex vertex = add_vertex(g);
        Node2Vertex[x] = vertex;
        Vertex2Node[vertex] = x;
      }
    }
    void removeNode(T x) {
      if (Node2Vertex.count(x) == 1) {
        remove_vertex(Node2Vertex[x], g);
        Vertex2Node.erase(Node2Vertex[x]);
        Node2Vertex.erase(x);
      }
    }

    /**
     * Remove node gently
     * 1. get in and out
     * 2. connect in to out, omit all labels
     * 3. remove node and in and out edges
     */
    void removeNodeGentle(T x) {
      if (hasNode(x)) {
        EdgeIter begin,end;
        boost::tie(begin, end) = edges(g);
        std::set<Vertex> outset;
        std::set<Vertex> inset;
        for (EdgeIter it=begin;it!=end;++it) {
          Edge e = *it;
          if (Node2Vertex[x] == source(e, g)) {
            outset.insert(target(e, g));
          }
          if (Node2Vertex[x] == target(e, g)) {
            inset.insert(source(e, g));
          }
        }
        // connect
        for (Vertex in : inset) {
          for (Vertex out : outset) {
            // addEdge(in, out);
            add_edge(in, out, g);
          }
        }
        // remove edge
        for (Vertex in : inset) {
          remove_edge(in, Node2Vertex[x], g);
        }
        for (Vertex out : outset) {
          remove_edge(Node2Vertex[x], out, g);
        }
        // remove node
        remove_vertex(Node2Vertex[x], g);
        Vertex2Node.erase(Node2Vertex[x]);
        Node2Vertex.erase(x);
      }
    }

    void removeOutEdge(T x) {
      if (Node2Vertex.count(x) == 1) {
        EdgeIter begin,end;
        boost::tie(begin, end) = edges(g);
        std::set<Vertex> toset;
        for (EdgeIter it=begin;it!=end;++it) {
          Edge e = *it;
          if (Node2Vertex[x] == source(e, g)) {
            // FIXME can i remove edge during iteration?
            // remove_edge(e, g);
            // T to = Vertex2Node[target(e, g)];
            toset.insert(target(e, g));
          }
        }
        // remvoe
        for (Vertex to : toset) {
          remove_edge(Node2Vertex[x], to, g);
        }
      }
    }

    void dump(std::ostream &os) {
      os << "Num of Vertices: " << num_vertices(g) << "\n";
      os << "Num of Edges: " << num_edges(g) << "\n";
      VertexIter begin,end;
      boost::tie(begin, end) = vertices(g);
      os << "Vertices:\n";
      for (VertexIter it=begin;it!=end;it++) {
        Vertex v = *it;
        os << v << ": ";
        // Vertex2Node[v]->dump(os);
        os << "\n";
      }
      os << "\n";
    }

    bool hasCycle() {
      bool has_cycle = false;
      cycle_detector vis(has_cycle);
      depth_first_search(g, visitor(vis));
      // cout << "The graph has a cycle? " << has_cycle << endl;
      return has_cycle;
    }

    std::vector<T> topoSort() {
      typedef std::list<Vertex> MakeOrder;
      MakeOrder::iterator i;
      MakeOrder make_order;

      topological_sort(g, std::front_inserter(make_order));
      // cout << "make ordering: ";
      // for (i = make_order.begin();
      //      i != make_order.end(); ++i) 
      //   cout << name[*i] << " ";
      
      std::vector<T> ret;
      for (Vertex v : make_order) {
        ret.push_back(Vertex2Node[v]);
      }
      return ret;
    }

    /**
     * visualize by exporting to graph
     */
    std::string getDotString(std::string (*labelFunc)(T));
    std::string getGgxString(std::string (*labelFunc)(T));

    void merge(Graph<T> &rhs);
  private:
    std::map<T,Vertex> Node2Vertex;
    std::map<Vertex,T> Vertex2Node;
    // adjacency_list<vecS, vecS, bidirectionalS> g;
    GraphType g;
  };

  template <typename T> std::string visualize(Graph<T> g);

  // /**
  //  * Connect graph -> graph
  //  */
  // friend template <typename T>
  // Graph<T> connect(Graph<T> from, std::set<T> fromnodes,
  //                  Graph<T> to, std::set<T> tonodes,
  //                  std::string label="");
  // /**
  //  * Connect graph -> node
  //  */
  // friend template <typename T>
  // Graph<T> connect(Graph<T> from, std::set<T> fromnodes,
  //                  std::set<T> to, std::string label="");
  // /**
  //  * Connect node -> graph
  //  */
  // friend template <typename T>
  // Graph<T> connect(std::set<T> from,
  //                  Graph<T> to, std::set<T> tonodes,
  //                  std::string label="");
}

#endif /* GRAPH_H */
