#include "HCALPFG/FCDAnalysis/interface/FCDAnalysis.h"

#include "FWCore/Framework/interface/ConstProductRegistry.h"
#include "FWCore/Framework/interface/ProductSelector.h"
#include "FWCore/Framework/interface/ProductSelectorRules.h"
#include "DataFormats/Provenance/interface/SelectedProducts.h"
#include "Math/LorentzVector.h"
#include "Math/Vector3D.h"

#include <map>
#include "boost/foreach.hpp"
#include <TBranch.h>
#include <TLorentzVector.h>


FCDAnalysis::FCDAnalysis(const edm::ParameterSet& ps) {
	//	tags
	_tagQIE10 = ps.getUntrackedParameter<edm::InputTag>("tagQIE10",
		edm::InputTag("hcalDigis", "ZDC"));
	_tokQIE10 = consumes<QIE10DigiCollection>(_tagQIE10);

	// channels
	edm::ParameterSet channelPSet = ps.getParameter<edm::ParameterSet>("fcdChannels");
	std::vector<int32_t> crates = channelPSet.getUntrackedParameter<std::vector<int32_t> >("crate");
	std::vector<int32_t> slots = channelPSet.getUntrackedParameter<std::vector<int32_t> >("slot");
	std::vector<int32_t> fibers = channelPSet.getUntrackedParameter<std::vector<int32_t> >("fiber");
	std::vector<int32_t> fiberChannels = channelPSet.getUntrackedParameter<std::vector<int32_t> >("fiber_channel");
	for (unsigned int i = 0; i < crates.size(); ++i) {
		_channels.push_back({crates[i], slots[i], fibers[i], fiberChannels[i]});
	}
}

FCDAnalysis::~FCDAnalysis() {
}

std::unique_ptr<FCDAnalysisStreamData> FCDAnalysis::beginStream(edm::StreamID sid) const {
	//return std::unique_ptr<FCDAnalysis::ChannelHistogramMap>(new FCDAnalysis::ChannelHistogramMap());
	return std::unique_ptr<FCDAnalysisStreamData>(new FCDAnalysisStreamData());
}

void FCDAnalysis::initializeStreamData(FCDAnalysisStreamData* streamData, edm::EventSetup const& es) const {
	// Get channels from emap
	edm::ESHandle<HcalDbService> dbs;
	es.get<HcalDbRecord>().get(dbs);
	HcalElectronicsMap const* emap = dbs->getHcalMapping();
	streamData->_ehashmap.initialize(emap, hcaldqm::electronicsmap::fD2EHashMap);

	std::vector<HcalGenericDetId> gids = emap->allPrecisionId();
	for (auto& it_gid : gids) {
		if (it_gid.genericSubdet() != HcalGenericDetId::HcalGenZDC) {
			continue;
		}
		HcalElectronicsId eid = HcalElectronicsId(streamData->_ehashmap.lookup(it_gid));
		for (auto& it_channel : _channels) {
			if ((eid.crateId() == it_channel.crate) && (eid.slot() == it_channel.slot) && (eid.fiberIndex() == it_channel.fiber) && (eid.fiberChanId() == it_channel.fiberChannel)) {
				streamData->_fcd_eids.push_back(eid);
			}
		}
	}

	streamData->_nevents = 0;

	std::string hname;	
	for (auto& it_eid : streamData->_fcd_eids) {
		// EM Pos
		hname = "h_ADC_" + std::to_string(it_eid.crateId()) + "-" + std::to_string(it_eid.slot()) + "-" + std::to_string(it_eid.fiberIndex()) + "-" + std::to_string(it_eid.fiberChanId());
		streamData->_hist_adc[it_eid.rawId()] = new TH1F(hname.c_str(), hname.c_str(), 256, -0.5, 255.5);
		streamData->_hist_adc[it_eid.rawId()]->GetXaxis()->SetTitle("ADC");

		hname = "h_ADC_vs_TS" + std::to_string(it_eid.crateId()) + "-" + std::to_string(it_eid.slot()) + "-" + std::to_string(it_eid.fiberIndex()) + "-" + std::to_string(it_eid.fiberChanId());
		streamData->_hist_adc_vs_ts[it_eid.rawId()] = new TH2F( hname.c_str(), hname.c_str(), 10, 0, 10, 64, -0.5, 255.5);
		streamData->_hist_adc_vs_ts[it_eid.rawId()]->GetXaxis()->SetTitle("TS");
		streamData->_hist_adc_vs_ts[it_eid.rawId()]->GetYaxis()->SetTitle("ADC");

		hname = "h_TDC_" + std::to_string(it_eid.crateId()) + "-" + std::to_string(it_eid.slot()) + "-" + std::to_string(it_eid.fiberIndex()) + "-" + std::to_string(it_eid.fiberChanId());
		streamData->_hist_tdc[it_eid.rawId()] = new TH1F(hname.c_str(), hname.c_str(), 64, -0.5, 63.5);
		streamData->_hist_tdc[it_eid.rawId()]->GetXaxis()->SetTitle("TDC");

		hname = "h_TDCTime_" + std::to_string(it_eid.crateId()) + "-" + std::to_string(it_eid.slot()) + "-" + std::to_string(it_eid.fiberIndex()) + "-" + std::to_string(it_eid.fiberChanId());
		streamData->_hist_tdctime[it_eid.rawId()] = new TH1F(hname.c_str(), hname.c_str(), 500, 0., 250.);
		streamData->_hist_tdctime[it_eid.rawId()]->GetXaxis()->SetTitle("TDCTime [ns]");

		hname = "h_ADC_vs_TDC_" + std::to_string(it_eid.crateId()) + "-" + std::to_string(it_eid.slot()) + "-" + std::to_string(it_eid.fiberIndex()) + "-" + std::to_string(it_eid.fiberChanId());
		streamData->_hist_adc_vs_tdc[it_eid.rawId()] = new TH2F(hname.c_str(), hname.c_str(), 64, -0.5, 63.5, 256, -0.5, 255.5);
		streamData->_hist_adc_vs_tdc[it_eid.rawId()]->GetXaxis()->SetTitle("TDC");
		streamData->_hist_adc_vs_tdc[it_eid.rawId()]->GetYaxis()->SetTitle("ADC");

		hname = "h_ADC_vs_TDCTime_" + std::to_string(it_eid.crateId()) + "-" + std::to_string(it_eid.slot()) + "-" + std::to_string(it_eid.fiberIndex()) + "-" + std::to_string(it_eid.fiberChanId());
		streamData->_hist_adc_vs_tdctime[it_eid.rawId()] = new TH2F(hname.c_str(), hname.c_str(), 500, 0., 250., 256, -0.5, 255.5);
		streamData->_hist_adc_vs_tdctime[it_eid.rawId()]->GetXaxis()->SetTitle("TDCTime [ns]");
		streamData->_hist_adc_vs_tdctime[it_eid.rawId()]->GetYaxis()->SetTitle("ADC");
	}

}

std::shared_ptr<FCDAnalysisStreamData> FCDAnalysis::globalBeginRunSummary(edm::Run const& run, edm::EventSetup const& es) const {
	std::shared_ptr<FCDAnalysisStreamData> returnValue(new FCDAnalysisStreamData());

	FCDAnalysis::initializeStreamData(returnValue.get(), es);

	return returnValue;
}

void FCDAnalysis::streamBeginRun(edm::StreamID sid, edm::Run const& run, edm::EventSetup const& es) const {
	FCDAnalysis::initializeStreamData(streamCache(sid), es);
}

void FCDAnalysis::beginJob() {}

void FCDAnalysis::analyze(edm::StreamID sid, const edm::Event& event, const edm::EventSetup& es) const {
	streamCache(sid)->_nevents += 1;

	edm::Handle<QIE10DigiCollection> digis;
	if (!event.getByToken(_tokQIE10, digis))
		edm::LogError("Collection QIE10DigiCollection for ZDC isn't available"
				+ _tagQIE10.label() + " " + _tagQIE10.instance());

	for ( auto it = digis->begin(); it != digis->end(); it++ ) {
		const QIE10DataFrame digi = static_cast<const QIE10DataFrame>(*it);
		HcalGenericDetId const& gdid = digi.detid();
		HcalElectronicsId eid = HcalElectronicsId(streamCache(sid)->_ehashmap.lookup(gdid));
		if (std::find(streamCache(sid)->_fcd_eids.begin(), streamCache(sid)->_fcd_eids.end(), eid) == streamCache(sid)->_fcd_eids.end()) {
			continue;
		}

		for ( int i = 0; i < digi.samples(); i++ ) {
			// iter over all samples
			streamCache(sid)->_hist_adc.at(eid.rawId())->Fill(digi[i].adc());
			streamCache(sid)->_hist_adc_vs_ts.at(eid.rawId())->Fill(i, digi[i].adc());
			streamCache(sid)->_hist_tdc.at(eid.rawId())->Fill(digi[i].le_tdc());
			streamCache(sid)->_hist_adc_vs_tdc.at(eid.rawId())->Fill(digi[i].le_tdc(), digi[i].adc());
			if (digi[i].le_tdc() <= 50.) {
				double tdctime = 25. * i + 0.5 * digi[i].le_tdc();
				streamCache(sid)->_hist_tdctime.at(eid.rawId())->Fill(tdctime);
				streamCache(sid)->_hist_adc_vs_tdctime.at(eid.rawId())->Fill(tdctime, digi[i].adc());
			}
		}
	}
}

void FCDAnalysis::streamEndRunSummary(edm::StreamID sid, edm::Run const& run, edm::EventSetup const& es, FCDAnalysisStreamData* globalData) const {
	//Add the values seen in this Stream to the total for this Run
	globalData->add(streamCache(sid));
	streamCache(sid)->reset();
}

void FCDAnalysis::globalEndRunSummary(edm::Run const& run, edm::EventSetup const& es, FCDAnalysisStreamData* globalData) const {
	// Save all histograms
	_queue.pushAndWait([&]() {
		_fs->file().cd();
		std::string subdir_name = "run" + std::to_string(run.runAuxiliary().run());
		TDirectory* subdir = _fs->file().mkdir(subdir_name.c_str());
		subdir->cd();
		for (auto& it : globalData->_hist_adc) {
			it.second->Write();
		}
		for (auto& it : globalData->_hist_adc_vs_ts) {
			it.second->Write();
		}
		for (auto& it : globalData->_hist_tdc) {
			it.second->Write();
		}
		for (auto& it : globalData->_hist_tdctime) {
			it.second->Write();
		}
	});
}
