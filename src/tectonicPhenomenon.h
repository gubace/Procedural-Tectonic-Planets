#pragma once
#include <cstdint>
#include <string>


class TectonicPhenomenon {
    public:
        enum class Type : uint8_t {
            Subduction,
            ContinentalCollision,
            Rifting
        };

        TectonicPhenomenon(Type t, unsigned int plateA, unsigned int plateB, unsigned int vertexIndex)
            : type(t), plate_a(plateA), plate_b(plateB), vertex_index(vertexIndex) {}

        virtual ~TectonicPhenomenon() = default;

        // Accès commun aux informations de base
        Type getType() const { return type; }
        unsigned int getPlateA() const { return plate_a; }
        unsigned int getPlateB() const { return plate_b; }
        unsigned int getVertexIndex() const { return vertex_index; }

        // Méthode virtuelle pure pour obtenir une description spécifique
        virtual std::string getDescription() const = 0;

    protected:
        Type type;
        unsigned int plate_a;
        unsigned int plate_b;
        unsigned int vertex_index;
};

class Subduction : public TectonicPhenomenon {
    public:
        enum class SubductionType : uint8_t {
            Oceanic_Oceanic,
            Oceanic_Continental,
            Continental_Continental
        };

        Subduction(unsigned int plateA, unsigned int plateB, unsigned int vertexIndex,
                unsigned int plateUnder, unsigned int plateOver, float convergenceRate,
                SubductionType subductionType, const std::string& reason)
            : TectonicPhenomenon(Type::Subduction, plateA, plateB, vertexIndex),
            plate_under(plateUnder), plate_over(plateOver), convergence(convergenceRate),
            subduction_type(subductionType), reason(reason) {}

        unsigned int getPlateUnder() const { return plate_under; }
        unsigned int getPlateOver() const { return plate_over; }
        float getConvergence() const { return convergence; }
        SubductionType getSubductionType() const { return subduction_type; }

        std::string getDescription() const override {
            return "Subduction: " + reason + " (Convergence: " + std::to_string(convergence) + ")";
        }

    private:
        unsigned int plate_under;
        unsigned int plate_over;
        float convergence;
        SubductionType subduction_type;
        std::string reason;
};


class ContinentalCollision : public TectonicPhenomenon {
    public:
        ContinentalCollision(unsigned int plateA, unsigned int plateB, unsigned int vertexIndex,
                            float collisionMagnitude, const std::string& description)
            : TectonicPhenomenon(Type::ContinentalCollision, plateA, plateB, vertexIndex),
            magnitude(collisionMagnitude), description(description) {}

        float getMagnitude() const { return magnitude; }

        std::string getDescription() const override {
            return "Continental Collision: " + description + " (Magnitude: " + std::to_string(magnitude) + ")";
        }

    private:
        float magnitude;
        std::string description;
};

class Rifting : public TectonicPhenomenon {
    public:
        Rifting(unsigned int plateA, unsigned int plateB, unsigned int vertexIndex,
                float divergenceRate, const std::string& description)
            : TectonicPhenomenon(Type::Rifting, plateA, plateB, vertexIndex),
            divergence(divergenceRate), description(description) {}

        float getDivergence() const { return divergence; }

        std::string getDescription() const override {
            return "Rifting: " + description + " (Divergence: " + std::to_string(divergence) + ")";
        }

    private:
        float divergence;
        std::string description;
};