A Kaldi package for ROS

How to use

0. Requirement
(1) install Jasper to ~/src
(2) install Sirius to ~/src
(3) install kaldi  to ~/src


1. Preparation
(1) cd ~/myprog/src/kaldi/script
(2) ln -s /home/demulab/src/sirius/sirius-application/speech-recognition/kaldi/script\
s/graph graph
(3) ln -s /home/demulab/src/sirius/sirius-application/speech-recognition/kaldi/scripts/nnet_a_gpu_online  nnet_a_gpu_online


2. Execute
(1) cd ~/myprog
(2) catkin_make
(3) Just execute the following command
    ~/myprog/src/kaldi/scritp/start-asr-robocup-server.py


