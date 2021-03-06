#include "ISpy/Analyzers/interface/ISpyPackedCandidate.h"
#include "ISpy/Analyzers/interface/ISpyService.h"
#include "ISpy/Analyzers/interface/ISpyVector.h"
#include "ISpy/Analyzers/interface/ISpyTrackRefitter.h"


#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/ESHandle.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ServiceRegistry/interface/Service.h"
#include "FWCore/Utilities/interface/Exception.h"

#include "ISpy/Services/interface/IgCollection.h"

#include "TrackingTools/TransientTrack/interface/TransientTrackBuilder.h"
#include "TrackingTools/Records/interface/TransientTrackRecord.h"

using namespace edm::service;
using namespace edm;

#include <iostream>

ISpyPackedCandidate::ISpyPackedCandidate(const edm::ParameterSet& iConfig)
: inputTag_(iConfig.getParameter<edm::InputTag>("iSpyPackedCandidateTag"))
{
  candidateToken_ = consumes<pat::PackedCandidateCollection>(inputTag_);
}

void ISpyPackedCandidate::analyze(const edm::Event& event, const edm::EventSetup& eventSetup)
{
  edm::Service<ISpyService> config;

  if ( ! config.isAvailable() )
  {
    throw cms::Exception ("Configuration")
      << "ISpyPackedCandidate requires the ISpyService\n"
     "which is not present in the configuration file.\n"
     "You must add the service in the configuration file\n"
     "or remove the module that requires it";
  }

  IgDataStorage *storage = config->storage();

  edm::Handle<pat::PackedCandidateCollection> collection;
  event.getByToken(candidateToken_, collection);

  edm::ESHandle<TransientTrackBuilder> ttB;
  eventSetup.get<TransientTrackRecord>().get("TransientTrackBuilder", ttB);

  if ( collection.isValid() )
  {
    std::string product = "PackedCandidates "
                          + edm::TypeID (typeid (pat::PackedCandidateCollection)).friendlyClassName() + ":"
                          + inputTag_.label() + ":"
                          + inputTag_.instance() + ":"
                          + inputTag_.process();

    IgCollection& products = storage->getCollection("Products_V1");
    IgProperty PROD = products.addProperty("Product", std::string ());
    IgCollectionItem item = products.create();
    item[PROD] = product;

    IgCollection &tracks = storage->getCollection("Tracks_V4");
    IgProperty VTX = tracks.addProperty("pos", IgV3d());
    IgProperty P   = tracks.addProperty("dir", IgV3d());
    IgProperty PT  = tracks.addProperty("pt", 0.0); 
    IgProperty PHI = tracks.addProperty("phi", 0.0);
    IgProperty ETA = tracks.addProperty("eta", 0.0);
    IgProperty CHARGE = tracks.addProperty("charge", int(0));
    IgProperty CHI2 = tracks.addProperty("chi2", 0.0);
    IgProperty NDOF = tracks.addProperty("ndof", 0.0);

    IgCollection &extras = storage->getCollection("Extras_V1");
    IgProperty IPOS = extras.addProperty("pos_1", IgV3d());
    IgProperty IP   = extras.addProperty("dir_1", IgV3d());
    IgProperty OPOS = extras.addProperty("pos_2", IgV3d());
    IgProperty OP   = extras.addProperty("dir_2", IgV3d());
    IgAssociations &trackExtras = storage->getAssociations("TrackExtras_V1");

    for ( pat::PackedCandidateCollection::const_iterator c = collection->begin(); 
          c != collection->end(); ++c )
    {

      if ( ! (*c).bestTrack() )
        continue;

      reco::TransientTrack tt = (*ttB).build((*c).bestTrack());

      TrajectoryStateOnSurface inner_ts = tt.innermostMeasurementState();
      TrajectoryStateOnSurface outer_ts = tt.outermostMeasurementState();

      if ( ! inner_ts.isValid() || ! outer_ts.isValid() ) 
      {
        std::cout<<"Not OK :( "<<std::endl;
        continue;
      }
      else 
      {
        std::cout<<"OK!"<<std::endl;
      }
      
        
      
      IgCollectionItem track = tracks.create();

      track[VTX] = IgV3d((*c).vx()/100.,
                         (*c).vy()/100.,
                         (*c).vz()/100.);

      IgV3d dir = IgV3d((*c).px(),
                        (*c).py(),
                        (*c).pz());

      ISpyVector::normalize(dir);
      track[P] = dir;

      track[PT] = (*c).pt();
      track[ETA] = (*c).eta();
      track[PHI] = (*c).phi();
      track[CHARGE] = (*c).charge();
      track[CHI2] = (*c).vertexChi2();
      track[NDOF] = (*c).vertexNdof();

      IgCollectionItem eitem = extras.create();

      /*
      eitem[IPOS] = IgV3d(inner_ts.globalPosition().x()/100.,
                          inner_ts.globalPosition().y()/100.,
                          inner_ts.globalPosition().z()/100.);
      */

      eitem[IPOS] = IgV3d();
      eitem[IP] = IgV3d();

      /*
      eitem[OPOS] = IgV3d(outer_ts.globalPosition().x()/100., 
                          outer_ts.globalPosition().y()/100., 
                          outer_ts.globalPosition().z()/100.);
      */

      eitem[OPOS] = IgV3d();
      eitem[OP] = IgV3d();
      
      trackExtras.associate(item,eitem);
       
    }
  }

  else
  {
    std::string error = "### Error: PackedCandidates "
                        + edm::TypeID (typeid (pat::PackedCandidateCollection)).friendlyClassName() + ":"
                        + inputTag_.label() + ":"
                        + inputTag_.instance() + " are not found";
    config->error(error);
  }
}

DEFINE_FWK_MODULE(ISpyPackedCandidate);
