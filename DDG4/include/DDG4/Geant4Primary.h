// $Id: Geant4Data.h 513 2013-04-05 14:31:53Z gaede $
//====================================================================
//  AIDA Detector description implementation
//--------------------------------------------------------------------
//
//  Author     : M.Frank
//
//====================================================================
#ifndef DD4HEP_GEANT4PRIMARY_H
#define DD4HEP_GEANT4PRIMARY_H

// Framework include files
#include "DDG4/Geant4Vertex.h"
#include "DDG4/Geant4Particle.h"

// C/C++ include files
#include <set>
#include <map>
#include <vector>
#include <memory>

// Forward declarations
class G4PrimaryParticle;

/*
 *   DD4hep namespace declaration
 */
namespace DD4hep {

  /*
   *   Simulation namespace declaration
   */
  namespace Simulation {

    // Forward declarations
    class Geant4Particle;
    class Geant4Vertex;
    
    class PrimaryExtension  {
    public:
      /// Default constructor
      PrimaryExtension() {}
      /// Default destructor
      virtual ~PrimaryExtension();      
    };

    /// Data structure to map primaries to particles.
    /**
     *  This data structure is added to the Geant4Event data extensions
     *  by the Geant4GenerationInit action.
     *
     *  @author  M.Frank
     *  @version 1.0
     */
    class Geant4PrimaryMap  {
    public:
      typedef std::map<const G4PrimaryParticle*,Geant4Particle*> Primaries;
      /// Mapping of primary particles of this event
      Primaries primaryMap; //! not persistent
      /// Default constructor
      Geant4PrimaryMap() {}
      /// Default destructor
      virtual ~Geant4PrimaryMap();
    };
    

    /// Class modelling a single interaction with multiple primary vertices and particles
    /*
     *  @author  M.Frank
     *  @version 1.0
     */
    class Geant4PrimaryInteraction   {
    private:
      /// Copy constructor
      Geant4PrimaryInteraction(const Geant4PrimaryInteraction& c);
      /// Assignment operator
      Geant4PrimaryInteraction& operator=(const Geant4PrimaryInteraction& c);

    public:
      typedef Geant4Particle                  Particle;
      typedef Geant4Vertex                    Vertex;
      typedef std::map<int,Particle*>         ParticleMap;
      typedef std::map<int,Vertex*>           VertexMap;
      typedef std::auto_ptr<PrimaryExtension> ExtensionHandle;

      /// The map of primary vertices for the particles.
      VertexMap       vertices;
      /// The map of particles participating in this primary interaction
      ParticleMap     particles;
      /// User data extension if required
      ExtensionHandle extension;
      /// User mask to flag the interaction. Also unique identifier
      int             mask;
      /// Next PID indentifier
      int             next_particle_identifier;

    public:
      /// Default constructor
      Geant4PrimaryInteraction();
      /// Default destructor
      virtual ~Geant4PrimaryInteraction();
      /// Access a new particle identifier within the interaction
      int nextPID();
      /// Set the next PID value
      void setNextPID(int value);
      /// Apply mask to all contained vertices (max. 1) and particles
      bool applyMask();
    };

    /// Class modelling a complete primary event with multiple interactions
    /*
     *  Multiple interactions allow a simple handling of overlay events
     *
     *  @author  M.Frank
     *  @version 1.0
     */
    class Geant4PrimaryEvent   {
    private:
      /// Copy constructor
      Geant4PrimaryEvent(const Geant4PrimaryEvent& c);
      /// Assignment operator
      Geant4PrimaryEvent& operator=(const Geant4PrimaryEvent& c);

    public:
      typedef Geant4PrimaryInteraction        Interaction;
      typedef std::map<int,Interaction*>      Interactions;
      typedef std::auto_ptr<PrimaryExtension> ExtensionHandle;

    protected:
      /// Set of primary interactions
      Interactions    m_interactions;

    public:
      /// User data extension if required
      ExtensionHandle extension;

    public:
      /// Default constructor
      Geant4PrimaryEvent();
      /// Default destructor
      virtual ~Geant4PrimaryEvent();
      /// Add a new interaction object to the event
      void add(int id, Geant4PrimaryInteraction* interaction);
      /// Retrieve an interaction by it's ID
      Geant4PrimaryInteraction* get(int id) const;
      /// Retrieve all intractions
      std::vector<Geant4PrimaryInteraction*> interactions() const;
    };

  }    // End namespace Simulation
}      // End namespace DD4hep
#endif // DD4HEP_GEANT4PRIMARY_H