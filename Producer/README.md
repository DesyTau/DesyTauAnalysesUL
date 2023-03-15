# NTuple Producer

This setup runs on miniAOD ntuples and uses [grid-control](https://github.com/grid-control/grid-control) for job handling.

## Current Campaing

For practical purposes please add here the link to the google doc with the datasets being produced when a new campaign is called:
Oktoberfest2021 campaign: [google doc](https://docs.google.com/spreadsheets/d/1MVDy3fOna8GEaYnzBEowtchffwRxakT-ilgZ25H1eRQ/edit?usp=sharing)


## Setup

Please make sure you first run the setup of the DesyTauAnalyses repository following the instructions in the [main directory](https://github.com/DesyTau/DesyTauAnalysesUL#desytauanalysesul).

To run the NTupleMaker on the batch system we recommend the use of [grid-control](https://github.com/grid-control/grid-control).
Please set up grid-control outside your CMSSW area.
```bash
git clone https://github.com/grid-control/grid-control.git -b testing
```

You will also have to setup [Rucio]() to access the miniAODs and run the NTuple production.
Please add you CERN username in `Producer/test/setupRucio.sh` and then execute it with `source setupRucio.sh`.
NOTE: do NOT set up your CMS environment before running `source setupRucio.sh`, the default python libraries of CMSSW are in conflict with the one used by Rucio.


## Running NTuple Production

The main script you will use is `/path/to/grid-control/go.py`.

First you need to check that the datasets are correctly stored in T2_DE_DESY with Rucio.
NOTE: You need to NOT have set up your CMSSW environment for this or there will be conflicts with the python library used by Rucio.

The method to request a Rucio transfer to create a replica in T2_DE_DESY of CMS dataset was changed in late 2022.
Updates on the procedures can be found in the [HamburgWikiComputingRucio twiki](https://twiki.cern.ch/twiki/bin/viewauth/CMS/HamburgWikiComputingRucio).

At the moment the only reliable way to request a Rucio replica is:

```bash
rucio add-rule --ask-approval --lifetime 2592000 cms:/DATASETNAME1 cms:/DATASETNAME2 [other datasets] 1 T2_DE_DESY
```


<details>
<summary>Deprecated Rucio tranfer procedure</summary>

The script `Producer/test/rucio_transfer.sh` can be used for transfering one dataset at a time, it can be executed with:
```bash
./rucio_transfer.sh <nickname of dataset> </DATASET-NAME/CAMPAIGN/MINIAODSIM(MINIAOD/USER)>
```
The script can easily be edited in order to attach multiple datasets to the same Rucio container and request the transfer of multiple datasets at once:
```bash
#!/bin/bash
NICK=$1
DATASET=$2
#Uncomment the following lines to add multiple datasets to the same container and request only one transfer
#DATASET_2=$3
#DATASET_3=$4

rucio add-container user.${RUCIO_ACCOUNT}:/Analyses/$NICK/USER 
rucio attach user.${RUCIO_ACCOUNT}:/Analyses/$NICK/USER cms:$DATASET
#rucio attach user.${RUCIO_ACCOUNT}:/Analyses/$NICK/USER cms:$DATASET_2
#rucio attach user.${RUCIO_ACCOUNT}:/Analyses/$NICK/USER cms:$DATASET_3
rucio add-rule --ask-approval user.${RUCIO_ACCOUNT}:/Analyses/$NICK/USER 1 T2_DE_DESY --lifetime 2592000 --asynchronous

```
The `attach user` instruction can be executed again for different datasets, in order to attach them to the same container.

</details>


Second: create a list of files from cms-das on which grid-control will act.
This can be done using the script `Producer/test/read_filelist_from_das.py`, to run it use (requires python libraries in CMSSW):
```bash
cd $CMSSW_BASE
cmsenv
cd $CMSSW_BASE/src/Producer/test
python read_filelist_from_das.py --nick <nickname of dataset> --query </DATASET-NAME/CAMPAIGN/MINIAODSIM(MINIAOD/USER)> --outputfile <my_list>
```

Then copy the grid-control config files stored in `Producer/test` to the area where you want to run the NTuple production, we recommend to choose an area outside of CMSSW for this purpose.

The config files are named `gc_DATA.cfg` and `gc_MC.cfg`, the areas to be edited before running are:
* `se path`: Path to storage area, simply put the full path to store on nfs or `srm://dcache-se-cms.desy.de:8443/srm/managerv2?SFN=/pnfs/desy.de/cms/tier2/store/user/<username>/<path to storage area>`
* `dbs instance`: (Needed only for embedded and private signal samples) The default location is `prod/global` (most MC/data are here), to switch to phys03 or other production instances uncomment this option and specify the type of instance used for the desired datasets
* `project area`: Input full path to your CMSSW area
* `dataset`: to use a file list provided with Rucio use `<nickname of dataset>: list:<full path to file list>`
* `nickname config`: python config file (`TreeProducer.py`) to be used for production.
* `nickname lumi filter`: Json lumi file turned into txt format, to be used when running on DATA.
* `events per job`: number of events stored in output tree. Recommended values
  * Signal samples: 10000 (recommended value)
  * Data/MC: 25000 (recommended value)

The following options can be changed also after grid-control has been initialized: 
* `wall time`: maximum runtime for the job, keep to 2:59:00 in order for the jobs to be submitted on the short queue. Increase in case jobs fail continuously with exit code **-1**
* `memory`: maximum memory usage for the job, keep 2000 in order for the jobs to be submitted on the short queue. Increase in case jobs fail continuously with exit code **-1**
* `max retry`: failed jobs are resubmitted automatically by grid-control, this automatic resubmission stops once they reach the maximum number of retries. Keep at 0 at the beginning, increase after fixing job failure cause. 



Copy the python config file to the area where you plan to run grid-control (again, please choose an area outside CMSSW), and edit the following variables:
* `isData`: enter True for data and leave false for MC (for Embedded it is automatically set to True
* `isEmbedded`: enter True only for Embedded samples, and not for real data
* `year`: actual year of data-taking, a number is expected (2016,2017 or 2018, for now)
* `period`: a string which marks the year and the data-processing iteration (UL2016APV, UL2016, UL2017 or UL2018). NOTE: the current setup does NOT work for ReReco and Legacy2016 campaigns.

Other variables are introduced for dedicated analyses, e.g. `isHiggsSignal` for STXS and `RunTauSpinnerProducer` for Higgs CP.

Suggestions: please rename python and grid-control config files before starting the ntuple production as it helps keeping track of what was used for a specific NTuple campaign.

Once everything is set up you can run grid-control with the following command
```bash
/path/to/grid-control/go.py -Gc gc_config.cfg
```
The options **-G** is used to activate the GUI and see the jobs submission and status being updated over time, while the option **-c** is used to run in continuous mode.

## FAQ on jobs failing

Several things can go wrong when submitting jobs with grid-control:

### Grid-control does not start
Possible reasons:
* In your local environment you use a different version of python with respect to the one used for grid-control.
  * Suggested solution: You can run `cmsenv` in a CMSSW environment, this should fix the python dependencies
* Proxy not found:
  * You probably either have not set up the CMS-VO proxy, or set it up in a directory which grid-control cannot access
  * Suggested solution: run `export X509_USER_PROXY=~/public/x509_voms; voms-proxy-init -voms cms -valid 48:00`

### Grid-control starts but jobs quickly move between RUNNING and ABORTED
* Before doing any check on the outputs try looking at the format of the date in your current locale
  * This is one of the most annoying things, when submitting a job to HTCondor the system is asked for the date and time BUT does not request any specific format for them, it just takes the default
  * What most likely it's happening is that either the **format** or the **language** of the date are incorrect:
  * Suggested solutions:
   * (Format problem) Check if you are using the *testing* branch of grid-control. If not, then try to clone again grid-control with `git clone https://github.com/grid-control/grid-control.git -b testing`
   * (Language problem) Check the language of the date with the command `date` or `echo $LC_TIME` if the date is not in English or the locale is set to anything but en_US.UTF-8 then run `export LC_TIME=en_US.UTF-8` and retry running grid-control

### Jobs fail
Check the exit code, here is a list of the most common ones and the suggested actions to take:
* **-1**: relaunch the job a few time, the most common cause of this error is that the job took too long to run or required more memory than the allocated one. It can also happen if the communication with the server where the dataset is stored is slow. If this persists after 3 failed attempts, just increase  `wall time` and `memory`
* **92**: problems in retrieving the dataset. Check using DAS if the dataset is completely replicated at T2_DE_DESY (an example of search query on das can be found [here](https://cmsweb.cern.ch/das/request?instance=prod/global&input=site+dataset%3D%2FGluGluHToTauTauUncorrelatedDecay_Filtered_M125_TuneCUETP8M1_13TeV-powheg-pythia8%2FRunIISummer16MiniAODv3-PUMoriond17_94X_mcRun2_asymptotic_v3-v1%2FMINIAODSIM)). File replica at T2_DE_DESY should be 100%, if it's not then request a rucio transfer.
* **106**: problems with writing the output files to the `se path`. Check if the directory you are trying to write to exists.
Other exit codes can be found at the following links:
* [twiki/CMSPublic/StandardExitCodes](https://twiki.cern.ch/twiki/bin/view/CMSPublic/StandardExitCodes)
* [grid-control official documentation](http://www-ekp.physik.uni-karlsruhe.de/~berger/gc/reference.html#error-codes)

