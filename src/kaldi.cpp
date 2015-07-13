// A Kaldi ROS package
// This program is based on following program
// onlinebin/online2-wav-nnet2-latgen-faster.cc.
// Copyright 2014  Johns Hopkins University (author: Daniel Povey)

// See ../../COPYING for clarification regarding multiple authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//  http://www.apache.org/licenses/LICENSE-2.0
//
// THIS CODE IS PROVIDED *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION ANY IMPLIED
// WARRANTIES OR CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR PURPOSE,
// MERCHANTABLITY OR NON-INFRINGEMENT.
// See the Apache 2 License for the specific language governing permissions and
// limitations under the License.

// FOR ROS
#include "ros/ros.h"
#include "std_msgs/String.h"
#include <sstream>


#include "feat/wave-reader.h"
#include "online2/online-nnet2-decoding.h"
#include "online2/onlinebin-util.h"
#include "online2/online-timing.h"
#include "online2/online-endpoint.h"
#include "fstext/fstext-lib.h"
#include "lat/lattice-functions.h"

namespace kaldi {

void GetDiagnosticsAndPrintOutput(const std::string &utt,
                                  const fst::SymbolTable *word_syms,
                                  const CompactLattice &clat,
                                  int64 *tot_num_frames,
                                  double *tot_like, ros::Publisher *recog_pub) {
  if (clat.NumStates() == 0) {
    KALDI_WARN << "Empty lattice.";
    return;
  }
  CompactLattice best_path_clat;
  CompactLatticeShortestPath(clat, &best_path_clat);
  
  Lattice best_path_lat;
  ConvertLattice(best_path_clat, &best_path_lat);
  
  double likelihood;
  LatticeWeight weight;
  int32 num_frames;
  std::vector<int32> alignment;
  std::vector<int32> words;
  GetLinearSymbolSequence(best_path_lat, &alignment, &words, &weight);
  num_frames = alignment.size();
  likelihood = -(weight.Value1() + weight.Value2());
  *tot_num_frames += num_frames;
  *tot_like += likelihood;
//  KALDI_VLOG(2) << "Likelihood per frame for utterance " << utt << " is "
//                << (likelihood / num_frames) << " over " << num_frames
//                << " frames.";
             
  std_msgs::String msg; // add demu
  std::stringstream ss;
  // ss << "transcript: ";
  if (word_syms != NULL) {
    for (size_t i = 0; i < words.size(); i++) {
      std::string s = word_syms->Find(words[i]);
      if (s == "") KALDI_ERR << "Word-id " << words[i] << " not in symbol table.";
      std::cout << s << ' ';
      ss << s << ' ';
    }
    std::cout << std::endl;
    //ss << std::endl;
  }
  std::cerr << ss.str();
  msg.data   = ss.str();
  recog_pub->publish(msg);  // add demu
}

}

int main(int argc, char *argv[]) {

  ros::init(argc, argv, "kaldi");
  ros::NodeHandle n;
  ros::Publisher recog_pub = n.advertise<std_msgs::String>("speech_recognition", 1000);
  ros::Rate loop_rate(10);


  try {
    using namespace kaldi;
    using namespace fst;
    
    typedef kaldi::int32 int32;
    typedef kaldi::int64 int64;
    
    const char *usage =
        "Reads in wav file(s) and simulates online decoding with neural nets\n"
        "(nnet2 setup), with optional iVector-based speaker adaptation and\n"
        "optional endpointing.  Note: some configuration values and inputs are\n"
        "set via config files whose filenames are passed as options\n"
        "\n"
        "Usage: online2-wav-nnet2-latgen-faster [options] <nnet2-in> <fst-in> "
        "<spk2utt-rspecifier> <wav-rspecifier> <lattice-wspecifier>\n"
        "The spk2utt-rspecifier can just be <utterance-id> <utterance-id> if\n"
        "you want to decode utterance by utterance.\n"
        "See egs/rm/s5/local/run_online_decoding_nnet2.sh for example\n"
        "See also online2-wav-nnet2-latgen-threaded\n";
    
    ParseOptions po(usage);
    
    std::string word_syms_rxfilename;
    
    OnlineEndpointConfig endpoint_config;

    // feature_config includes configuration for the iVector adaptation,
    // as well as the basic features.
    OnlineNnet2FeaturePipelineConfig feature_config;  
    OnlineNnet2DecodingConfig nnet2_decoding_config;

    BaseFloat chunk_length_secs = 0.05;
    bool do_endpointing = false;
    bool online = true;
    
    po.Register("chunk-length", &chunk_length_secs,
                "Length of chunk size in seconds, that we process.  Set to <= 0 "
                "to use all input in one chunk.");
    po.Register("word-symbol-table", &word_syms_rxfilename,
                "Symbol table for words [for debug output]");
    po.Register("do-endpointing", &do_endpointing,
                "If true, apply endpoint detection");
    po.Register("online", &online,
                "You can set this to false to disable online iVector estimation "
                "and have all the data for each utterance used, even at "
                "utterance start.  This is useful where you just want the best "
                "results and don't care about online operation.  Setting this to "
                "false has the same effect as setting "
                "--use-most-recent-ivector=true and --greedy-ivector-extractor=true "
                "in the file given to --ivector-extraction-config, and "
                "--chunk-length=-1.");
    
    feature_config.Register(&po);
    nnet2_decoding_config.Register(&po);
    endpoint_config.Register(&po);
    
    po.Read(argc, argv);
    
    if (po.NumArgs() != 5) {
      po.PrintUsage();
      return 1;
    }
    
    std::string nnet2_rxfilename = po.GetArg(1),
        fst_rxfilename = po.GetArg(2),
        spk2utt_rspecifier = po.GetArg(3),
        wav_rspecifier = po.GetArg(4),
        clat_wspecifier = po.GetArg(5);
    
    OnlineNnet2FeaturePipelineInfo feature_info(feature_config);

    if (!online) {
      feature_info.ivector_extractor_info.use_most_recent_ivector = true;
      feature_info.ivector_extractor_info.greedy_ivector_extractor = true;
      chunk_length_secs = -1.0;
    }
    
    TransitionModel trans_model;
    nnet2::AmNnet nnet;
    {
      bool binary;
      Input ki(nnet2_rxfilename, &binary);
      trans_model.Read(ki.Stream(), binary);
      nnet.Read(ki.Stream(), binary);
    }
  
    fst::Fst<fst::StdArc> *decode_fst = ReadFstKaldi(fst_rxfilename);
    
    fst::SymbolTable *word_syms = NULL;
    if (word_syms_rxfilename != "")
      if (!(word_syms = fst::SymbolTable::ReadText(word_syms_rxfilename)))
        KALDI_ERR << "Could not read symbol table from file "
                  << word_syms_rxfilename;
    
//    int32 num_done = 0, num_err = 0;
    int32 num_done = 0;
    double tot_like = 0.0;
    int64 num_frames = 0;
    
//    SequentialTokenVectorReader spk2utt_reader(spk2utt_rspecifier);
//    RandomAccessTableReader<WaveHolder> wav_reader(wav_rspecifier);
    CompactLatticeWriter clat_writer(clat_wspecifier);
    
    OnlineTimingStats timing_stats;
    
    // Time how many time spent on decoding 
    std::string wav_file_name;
    while(ros::ok() && cin >> wav_file_name){
        
	Timer timer_decoding; 
        std::cerr << "Wavfile name received. Decoding "<<wav_file_name << std::endl;
        std::ifstream WavFile(wav_file_name.c_str());
        WaveData wave_data;
        wave_data.Read(WavFile);

//        const WaveData &wave_data = wav_reader.Value(utt);
        // get the data for channel zero (if the signal is not mono, we only
        // take the first channel).
        SubVector<BaseFloat> data(wave_data.Data(), 0);

        OnlineNnet2FeaturePipeline feature_pipeline(feature_info);
//        feature_pipeline.SetAdaptationState(adaptation_state);
        
        SingleUtteranceNnet2Decoder decoder(nnet2_decoding_config,
                                            trans_model,
                                            nnet,
                                            *decode_fst,
                                            &feature_pipeline);
        OnlineTimer decoding_timer("utt1");
        
        BaseFloat samp_freq = wave_data.SampFreq();
        int32 chunk_length; if (chunk_length_secs > 0) {
          chunk_length = int32(samp_freq * chunk_length_secs);
          if (chunk_length == 0) chunk_length = 1;
        } else {
          chunk_length = std::numeric_limits<int32>::max();
        }
        
        int32 samp_offset = 0;
        while (samp_offset < data.Dim()) {
          int32 samp_remaining = data.Dim() - samp_offset;
          int32 num_samp = chunk_length < samp_remaining ? chunk_length
                                                         : samp_remaining;
          
          SubVector<BaseFloat> wave_part(data, samp_offset, num_samp);
          feature_pipeline.AcceptWaveform(samp_freq, wave_part);
          
          samp_offset += num_samp;
          decoding_timer.WaitUntil(samp_offset / samp_freq);
          if (samp_offset == data.Dim()) {
            // no more input. flush out last frames
            feature_pipeline.InputFinished();
          }
          
          decoder.AdvanceDecoding();
          if (do_endpointing && decoder.EndpointDetected(endpoint_config))
            break;
        }
//        decoder.FinalizeDecoding();

        CompactLattice clat;
        bool end_of_utterance = true;
        decoder.GetLattice(end_of_utterance, &clat);
        
        GetDiagnosticsAndPrintOutput("utt1", word_syms, clat,
                                     &num_frames, &tot_like, &recog_pub);
        
        decoding_timer.OutputStats(&timing_stats);
        double decoding_time = timer_decoding.Elapsed();       
        std::cerr<<"Time spent on this input: "<<decoding_time<<"s"<<std::endl<<std::endl;

        // In an application you might avoid updating the adaptation state if
        // you felt the utterance had low confidence.  See lat/confidence.h
//        feature_pipeline.GetAdaptationState(&adaptation_state);
        
        // we want to output the lattice with un-scaled acoustics.
//        BaseFloat inv_acoustic_scale =
//            1.0 / nnet2_decoding_config.decodable_opts.acoustic_scale;
//        ScaleLattice(AcousticLatticeScale(inv_acoustic_scale), &clat);

//        clat_writer.Write("utt1", clat);
//        KALDI_LOG << "Decoded utterance " << utt;
//        num_done++;
//      }
	ros::spinOnce();
	loop_rate.sleep();
    }

    timing_stats.Print(online);
    
    delete decode_fst;
    delete word_syms; // will delete if non-NULL.
    return (num_done != 0 ? 0 : 1);
  } catch(const std::exception& e) {
    std::cerr << e.what();
    return -1;
  }
} // main()
