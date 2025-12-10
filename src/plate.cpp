#include <unordered_set>
#include <queue>


#include "planet.h"
#include "SphericalGrid.h"


void Plate::fillTerranes(const Planet& planet) {
    terranes.clear();
    terraneCentroids.clear();
    std::unordered_set<unsigned int> continentalVertices;
    
    for (unsigned int vIdx : vertices_indices) {
        if (vIdx < planet.crust_data.size() && planet.crust_data[vIdx]) {
            ContinentalCrust* cc = dynamic_cast<ContinentalCrust*>(planet.crust_data[vIdx].get());
            if (cc) {
                continentalVertices.insert(vIdx);
            }
        }
    }
    
    if (continentalVertices.empty()) {
        std::cout << "  No continental crust found in this plate." << std::endl;
        return;
    }
    

    
    std::unordered_set<unsigned int> visited;
    
    
    for (unsigned int startVertex : continentalVertices) {
        if (visited.find(startVertex) != visited.end()) continue;
        
        // Créer une nouvelle terrane
        std::vector<unsigned int> terrane;
        std::vector<unsigned int> toExplore;
        
        toExplore.push_back(startVertex);
        visited.insert(startVertex);
        
        // Exploration en largeur (BFS) pour trouver tous les vertices connectés
        while (!toExplore.empty()) {
            unsigned int current = toExplore.back();
            toExplore.pop_back();
            terrane.push_back(current);
            
            // Explorer les voisins
            if (current < planet.neighbors.size()) {
                for (unsigned int neighbor : planet.neighbors[current]) {
                    // Vérifier si déjà visité
                    if (visited.find(neighbor) != visited.end()) continue;
                    
                    // Vérifier si c'est un vertex continental de cette plaque
                    if (continentalVertices.find(neighbor) != continentalVertices.end()) {
                        visited.insert(neighbor);
                        toExplore.push_back(neighbor);
                    }
                }
            }
        }
        

        if (terrane.size() >= 20) {

            Vec3 centroid(0.0f, 0.0f, 0.0f);


            for (unsigned int vIdx : terrane) {
                if (vIdx < planet.vertices.size()) {
                    centroid += planet.vertices[vIdx];
                } else {
                    std::cerr << "WARNING: Invalid vertex index " << vIdx 
                              << " in terrane (max: " << planet.vertices.size() - 1 << ")" << std::endl;
                }
            }

            centroid /= (float)terrane.size();
            
            centroid.normalize();
            centroid *= planet.radius;
            
            terranes.push_back(terrane);
            terraneCentroids.push_back(centroid);
        }
    }

    if (terranes.size() != terraneCentroids.size()) {
        std::cerr << "ERROR: Mismatch between terranes and centroids count!" << std::endl;
    }
    
}