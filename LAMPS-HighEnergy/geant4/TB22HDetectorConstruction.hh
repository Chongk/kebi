#ifndef TB22HDETECTORCONSTRUCTION_HH
#define TB22HDETECTORCONSTRUCTION_HH

#include "G4VUserDetectorConstruction.hh"
#include "globals.hh"
#include "G4Cache.hh"

class G4VPhysicalVolume;
class G4LogicalVolume;
class G4NistManager;
class G4VisAttributes;
class LHMagneticFieldSetup;

class TB22HDetectorConstruction : public G4VUserDetectorConstruction
{
	public:

		TB22HDetectorConstruction();
		virtual ~TB22HDetectorConstruction();
		virtual G4VPhysicalVolume* Construct();

	private:

		G4NistManager* fNist;
        G4Cache<LHMagneticFieldSetup*> fFieldCache; //BField
};
#endif
