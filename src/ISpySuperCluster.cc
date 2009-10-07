#include "ISpy/Analyzers/interface/ISpySuperCluster.h"
#include "ISpy/Analyzers/interface/ISpyService.h"

#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/ESHandle.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ServiceRegistry/interface/Service.h"
#include "FWCore/Utilities/interface/Exception.h"

#include "ISpy/Services/interface/IgCollection.h"

#include "Geometry/CaloGeometry/interface/CaloCellGeometry.h"
#include "Geometry/CaloGeometry/interface/CaloGeometry.h"
#include "Geometry/Records/interface/CaloGeometryRecord.h"

#include "DataFormats/EgammaReco/interface/SuperCluster.h"
#include "DataFormats/EgammaReco/interface/SuperClusterFwd.h"

using namespace edm::service;
using namespace edm;

ISpySuperCluster::ISpySuperCluster(const edm::ParameterSet& iConfig)
: inputTag_(iConfig.getParameter<edm::InputTag>("iSpySuperClusterTag")){}

void ISpySuperCluster::analyze(const edm::Event& event, const edm::EventSetup& eventSetup)
{
  edm::Service<ISpyService> config;

  if ( ! config.isAvailable() )
  {
    throw cms::Exception ("Configuration")
      << "ISpySuperCluster requires the ISpyService\n"
      "which is not present in the configuration file.\n"
      "You must add the service in the configuration file\n"
      "or remove the module that requires it";
  }
  IgDataStorage *storage = config->storage();

  edm::ESHandle<CaloGeometry> geom;
  eventSetup.get<CaloGeometryRecord>().get(geom);

  if ( ! geom.isValid() )
  {
    std::string error = 
      "### Error: ISpySuperCluster::analyze: Invalid CaloGeometryRecord ";

    IgCollection& collection = storage->getCollection("Errors_V1");
    IgProperty ERROR_MSG = collection.addProperty("Error", std::string());
    IgCollectionItem item = collection.create();
    item[ERROR_MSG] = error;

    return;
  }

  edm::Handle<reco::SuperClusterCollection> collection;
  event.getByLabel(inputTag_, collection);

  if ( collection.isValid() )
  {
    std::string product = "SuperClusters "
                          + edm::TypeID (typeid (reco::SuperClusterCollection)).friendlyClassName() + ":"
                          + inputTag_.label() + ":"
                          + inputTag_.instance() + ":"
                          + inputTag_.process();

    IgCollection& products = storage->getCollection("Products_V1");
    IgProperty PROD = products.addProperty("Product", std::string ());
    IgCollectionItem item = products.create();
    item[PROD] = product;

    IgCollection& clusters = storage->getCollection("SuperClusters_V1");

    IgProperty E    = clusters.addProperty("energy", 0.0);
    IgProperty POS  = clusters.addProperty("pos", IgV3d());
    IgProperty ETA  = clusters.addProperty("eta", 0.0);
    IgProperty PHI  = clusters.addProperty("phi", 0.0);
    IgProperty ALGO = clusters.addProperty("algo", std::string());
    IgProperty ETAW = clusters.addProperty("etaWidth", 0.0);
    IgProperty PHIW = clusters.addProperty("phiWidth", 0.0);
    IgProperty RAWE = clusters.addProperty("rawEnergy", 0.0);
    IgProperty PREE = clusters.addProperty("preshowerEnergy", 0.0);

    IgCollection& fractions = storage->getCollection("RecHitFractions_V1");
  
    IgProperty DETID   = fractions.addProperty("detid", int(0));
    IgProperty FRACT   = fractions.addProperty("fraction", 0.0);
    IgProperty FRONT_1 = fractions.addProperty("front_1", IgV3d());
    IgProperty FRONT_2 = fractions.addProperty("front_2", IgV3d());
    IgProperty FRONT_3 = fractions.addProperty("front_3", IgV3d());
    IgProperty FRONT_4 = fractions.addProperty("front_4", IgV3d());
    IgProperty BACK_1  = fractions.addProperty("back_1",  IgV3d());
    IgProperty BACK_2  = fractions.addProperty("back_2",  IgV3d());
    IgProperty BACK_3  = fractions.addProperty("back_3",  IgV3d());
    IgProperty BACK_4  = fractions.addProperty("back_4",  IgV3d());

    IgAssociationSet& superClustersFracs = storage->getAssociationSet("SuperClusterRecHitFractions_V1");

    for ( reco::SuperClusterCollection::const_iterator ci = collection->begin(), cie = collection->end();
          ci != cie; ++ci )
    {
      IgCollectionItem c = clusters.create();
      
      c[E] = (*ci).energy();
      c[POS] = IgV3d((*ci).x()/100.0, (*ci).y()/100.0, (*ci).z()/100.0);
      c[ETA] = (*ci).eta();
      c[PHI] = (*ci).phi();
      c[ALGO] = algoName((*ci).algo());
      c[ETAW] = (*ci).etaWidth();
      c[PHIW] = (*ci).phiWidth();
      c[RAWE] = (*ci).rawEnergy();
      c[PREE] = (*ci).preshowerEnergy();

      std::vector<std::pair<DetId, float> > hitsAndFractions = (*ci).hitsAndFractions();
      
      for ( std::vector<std::pair<DetId, float> >::iterator hi = 
              hitsAndFractions.begin(), hie = hitsAndFractions.end(); hi != hie; ++hi )
      {
        IgCollectionItem rhf = fractions.create();
        rhf[DETID] = (*hi).first;
        rhf[FRACT] = static_cast<double>((*hi).second);

        const CaloCellGeometry *cell = (*geom).getGeometry((*hi).first);
        const CaloCellGeometry::CornersVec& corners = cell->getCorners();
        
        assert(corners.size() == 8);
		
        rhf[FRONT_1] = IgV3d(corners[3].x()/100.0, corners[3].y()/100.0, corners[3].z()/100.0);
        rhf[FRONT_2] = IgV3d(corners[2].x()/100.0, corners[2].y()/100.0, corners[2].z()/100.0);
        rhf[FRONT_3] = IgV3d(corners[1].x()/100.0, corners[1].y()/100.0, corners[1].z()/100.0);
        rhf[FRONT_4] = IgV3d(corners[0].x()/100.0, corners[0].y()/100.0, corners[0].z()/100.0);
	    
        rhf[BACK_1] = IgV3d(corners[7].x()/100.0, corners[7].y()/100.0, corners[7].z()/100.0);
        rhf[BACK_2] = IgV3d(corners[6].x()/100.0, corners[6].y()/100.0, corners[6].z()/100.0);
        rhf[BACK_3] = IgV3d(corners[5].x()/100.0, corners[5].y()/100.0, corners[5].z()/100.0);
        rhf[BACK_4] = IgV3d(corners[4].x()/100.0, corners[4].y()/100.0, corners[4].z()/100.0);	
        
        superClustersFracs.associate(c, rhf);
      }	    
    }
  }  
  else
  {
    std::string error = "### Error: SuperClusters "
                        + edm::TypeID (typeid (reco::SuperClusterCollection)).friendlyClassName() + ":"
                        + inputTag_.label() + ":"
                        + inputTag_.instance() + ":"
                        + inputTag_.process() + " are not found.";

    IgCollection& collection = storage->getCollection("Errors_V1");
    IgProperty ERROR_MSG = collection.addProperty("Error", std::string());
    IgCollectionItem item = collection.create();
    item[ERROR_MSG] = error;
  }
}

const std::string ISpySuperCluster::algoName (reco::CaloCluster::AlgoId key)
{
  if ( key == reco::CaloCluster::island )
    return "island";
  else if ( key == reco::CaloCluster::hybrid )
    return "hybrid";
  else if ( key == reco::CaloCluster::fixedMatrix )   
    return "fixedMatrix";
  else if ( key == reco::CaloCluster::dynamicHybrid ) 
    return "dynamicHybrid";
  else if ( key == reco::CaloCluster::multi5x5 ) 
    return "multi5x5";
  else if ( key == reco::CaloCluster::particleFlow )
    return "particleFlow";
  else          
    return "undefined";
}

DEFINE_FWK_MODULE(ISpySuperCluster);
