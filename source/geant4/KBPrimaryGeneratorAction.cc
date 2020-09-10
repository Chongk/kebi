#include "KBPrimaryGeneratorAction.hh"
#include "KBG4RunManager.hh"

#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4ParticleDefinition.hh"
#include "G4SystemOfUnits.hh"
#include "G4PhysicalConstants.hh"
#include <G4strstreambuf.hh>

KBPrimaryGeneratorAction::KBPrimaryGeneratorAction()
{
  fParticleGun = new G4ParticleGun();
}

KBPrimaryGeneratorAction::KBPrimaryGeneratorAction(const char *fileName)
{
  fParticleGun = new G4ParticleGun();
	auto runManager = (KBG4RunManager *) G4RunManager::GetRunManager();
	auto par = runManager -> GetParameterContainer();
	if ( par->GetParInt("G4InputMode")==1 )
	{
		fEventGenerator = new KBMCEventGenerator(fileName);
		fReadMomentumOrEnergy = fEventGenerator -> ReadMomentumOrEnergy();
	}
}

KBPrimaryGeneratorAction::~KBPrimaryGeneratorAction()
{
  delete fParticleGun;
}

void KBPrimaryGeneratorAction::GeneratePrimaries(G4Event* anEvent)
{

	auto runManager = (KBG4RunManager *) G4RunManager::GetRunManager();
	auto par = runManager -> GetParameterContainer();
	if ( par->GetParInt("G4InputMode")==0 )
	{
		GeneratePrimariesMode0(anEvent);
	}
	else if ( par->GetParInt("G4InputMode")==1 )
	{
		GeneratePrimariesMode1(anEvent);
	}
}

void KBPrimaryGeneratorAction::GeneratePrimariesMode0(G4Event* anEvent)
{

	auto runManager = (KBG4RunManager *) G4RunManager::GetRunManager();
	auto par = runManager -> GetParameterContainer();

	G4double vx, vy, vz;

	vz = -1.0*par->GetParDouble("worlddZ");

	G4double energy = par->GetParDouble("G4InputEnergy");

	G4strstreambuf* oldBuffer = dynamic_cast<G4strstreambuf*>(G4cout.rdbuf(0));
	fParticleGun->SetParticleMomentumDirection(G4ThreeVector(0,0,1));
	fParticleGun->SetParticleEnergy(energy*MeV);
	G4cout.rdbuf(oldBuffer);

	G4ParticleTable* particleTable = G4ParticleTable::GetParticleTable();
	TString particleName = par->GetParString("G4InputParticle");
	G4ParticleDefinition* particle
		= particleTable->FindParticle(particleName.Data());
	fParticleGun->SetParticleDefinition(particle);

	G4int NperEvent = par->GetParInt("G4InputNumberPerEvent"); 

	for (G4int ip=0; ip<NperEvent; ip++){

		//10 cm by 10 cm (square)
		vx = (G4UniformRand()-0.5)*100;
		vy = (G4UniformRand()-0.5)*100;
		/*
		G4double r = (G4UniformRand())*15.0; 
		G4double phi = (G4UniformRand()-0.5)*twopi;

		vx = r*cos(phi);
		vy = r*sin(phi);
		*/

		fParticleGun -> SetParticlePosition(G4ThreeVector(vx,vy,vz));

    fParticleGun->GeneratePrimaryVertex(anEvent);

  }//ip
}

void KBPrimaryGeneratorAction::GeneratePrimariesMode1(G4Event* anEvent)
{

  G4int pdg;
  G4double vx, vy, vz, px, py, pz;

  fEventGenerator -> ReadNextEvent(vx, vy, vz);

  fParticleGun -> SetParticlePosition(G4ThreeVector(vx,vy,vz));

  while (fEventGenerator -> ReadNextTrack(pdg, px, py, pz))
  {
    G4ParticleDefinition* particle = G4ParticleTable::GetParticleTable() -> FindParticle(pdg);
    fParticleGun -> SetParticleDefinition(particle);

    G4ThreeVector momentum(px,py,pz);
    fParticleGun -> SetParticleMomentumDirection(momentum.unit());

    G4strstreambuf* oldBuffer = dynamic_cast<G4strstreambuf*>(G4cout.rdbuf(0));
    // Removing print outs in between here ------------->
    if (fReadMomentumOrEnergy) fParticleGun -> SetParticleMomentum(momentum.mag()*MeV);
    else                       fParticleGun -> SetParticleEnergy(momentum.mag()*MeV);
    // <------------- to here
    G4cout.rdbuf(oldBuffer);

    fParticleGun -> GeneratePrimaryVertex(anEvent);
  }

}

void KBPrimaryGeneratorAction::SetEventGenerator(const char *fileName)
{
  fEventGenerator = new KBMCEventGenerator(fileName);
  fReadMomentumOrEnergy = fEventGenerator -> ReadMomentumOrEnergy();
  ((KBG4RunManager *) KBG4RunManager::GetRunManager()) -> SetNumEvents(fEventGenerator -> GetNumEvents());
}
