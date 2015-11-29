#include <ChessboardSampling.hpp>
#include <calibvolume.hpp>
#include <rgbdsensor.hpp>
#include <DataTypes.hpp>
#include <CMDParser.hpp>
#include <stablesampler.hpp>
#include <calibrator.hpp>
#include <fstream>
#include <iostream>
#include <unistd.h>


int main(int argc, char* argv[]){

  std::string pose_offset_filename = "../../../source/poseoffset";
  float    max_shaking_speed = 0.0005; // speed in meter/frames
  unsigned min_num_frames_below_max_shaking = 30;
  unsigned num_frames_to_filter = 30;
  unsigned artport = 5000;
  unsigned arttargetid = 6;
  unsigned idwneighbours = 20;
  unsigned num_locations_to_sample = 1;
  CMDParser p("basefilename serverport");
  p.addOpt("p",1,"poseoffetfilename", "specify the filename where to store the poseoffset on disk, default: " + pose_offset_filename);
  p.addOpt("m",1,"max_shaking_speed", "use this maximum speed in meter/frame the checkerboard should have, default: 0.0005");
  p.addOpt("f",1,"min_num_frames_below_max_shaking", "use minimum number of frames the checkerboard should stay under max_shaking_speed, default: 30");
  p.addOpt("x",1,"num_frames_to_filter", "filter number of frames, default: 30");
  p.addOpt("a",1,"artport", "the port the A.R.T system is on the network, default: 5000");
  p.addOpt("i",1,"id", "the A.R.T target id of the checkerboard transform, default: 6");
  p.addOpt("n",1,"numneighbours", "the number of neighbours that should be used for IDW inverse distance weighting, default: 20");
  p.addOpt("s",1,"num_locations_to_sample", "specify how many board locations should be sampled, default: 1");

  p.init(argc,argv);

  if(p.isOptSet("p")){
    pose_offset_filename = p.getOptsString("p")[0];
    std::cout << "setting poseoffetfilename to " << pose_offset_filename << std::endl;
  }

  if(p.isOptSet("m")){
    max_shaking_speed = p.getOptsFloat("m")[0];
  }
  if(p.isOptSet("f")){
    min_num_frames_below_max_shaking = p.getOptsInt("f")[0];
  }
  if(p.isOptSet("x")){
    num_frames_to_filter = p.getOptsInt("x")[0];
  }
  if(p.isOptSet("a")){
    artport = p.getOptsInt("a")[0];
  }
  if(p.isOptSet("i")){
    arttargetid = p.getOptsInt("i")[0];
  }
  if(p.isOptSet("n")){
    idwneighbours = p.getOptsInt("n")[0];
  }
  if(p.isOptSet("s")){
    num_locations_to_sample = p.getOptsInt("s")[0];
  }

  std::string basefilename = p.getArgs()[0];
  std::string filename_xyz(basefilename + "_xyz");
  std::string filename_uv(basefilename + "_uv");


  CalibVolume cv(filename_xyz.c_str(), filename_uv.c_str());

  RGBDConfig cfg;
  cfg.size_rgb = glm::uvec2(1280, 1080);
  cfg.size_d   = glm::uvec2(512, 424);
  cfg.serverport = p.getArgs()[1];
  RGBDSensor sensor(cfg);

  Checkerboard cb;
  cb.load_pose_offset(pose_offset_filename.c_str());
  // configure local 3D Points on chessboard here:
  for(unsigned y = 0; y < CB_HEIGHT; ++y){
    for(unsigned x = 0; x < CB_WIDTH; ++x){
      cb.points_local.push_back(glm::vec3(y * 0.075, x * 0.075,0.0));
    }
  }

  //ChessboardSampling cs("/mnt/pitoti/tmp_steppo/23_sweep");
  //cs.init(true);
  //cs.dump();

  StableSampler ss(&sensor, &cv, artport, arttargetid, &cb);

  // sample board locations
  while(num_locations_to_sample > 0){
    --num_locations_to_sample;

    ss.sampleBoardLocation(max_shaking_speed, min_num_frames_below_max_shaking, num_frames_to_filter);

    // play sound to notice user and wait 5 seconds
    system("/usr/bin/aplay ../../../framework/click_x.wav");

    std::cout << "--------------------------------------------------------" << std::endl;
    std::cout << "----------------------> Locations left: " << num_locations_to_sample << std::endl;
    std::cout << "--------------------------------------------------------" << std::endl;

    sleep(5);
  }

  // ss.dumpSamplePoints();

  Calibrator    c;  
  c.applySamples(&cv, ss.getSamplePoints(), cfg, idwneighbours);
  cv.save(filename_xyz.c_str(), filename_uv.c_str());
  


  return 0;
}
