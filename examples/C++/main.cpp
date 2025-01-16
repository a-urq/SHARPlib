
#include <SHARPlib/constants.h>
#include <SHARPlib/layer.h>
#include <SHARPlib/parcel.h>
#include <SHARPlib/thermo.h>
#include <SHARPlib/winds.h>

#include <chrono>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

// splits a string on a delimiter and stores in a vector
std::vector<std::string> split(std::string& s, std::string delimiter) {
    size_t pos_start = 0;
    size_t pos_end = 0;
    size_t delim_len = delimiter.length();
    std::string token;
    std::vector<std::string> res;

    while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos) {
        token = s.substr(pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        res.push_back(token);
    }

    res.push_back(s.substr(pos_start));
    return res;
}

void build_profile(float* pres_arr, float*hght_arr, float* tmpk_arr, 
                   float* dwpk_arr, float* wdir_arr, float* wspd_arr, 
                   float* mixr_arr, float* vtmp_arr, float* theta_arr, 
                   float* theta_e_arr, float* uwin_arr, float* vwin_arr, 
                   int* prof_N, std::vector<std::string>& row, int idx) {
    float pres = std::stof(row[0]) * sharp::HPA_TO_PA;
    float hght = std::stof(row[1]);
    float tmpk = std::stof(row[2]) + sharp::ZEROCNK;
    float dwpk = std::stof(row[3]) + sharp::ZEROCNK;
    float wdir = std::stof(row[4]);
    float wspd = std::stof(row[5]);

    pres_arr[idx] = pres;
    hght_arr[idx] = hght;
    tmpk_arr[idx] = tmpk;
    dwpk_arr[idx] = dwpk;
    wdir_arr[idx] = wdir;
    wspd_arr[idx] = wspd;

    mixr_arr[idx] = sharp::mixratio(pres, dwpk);
    vtmp_arr[idx] = sharp::virtual_temperature(tmpk, mixr_arr[idx]);
    theta_arr[idx] = sharp::theta(pres, tmpk, sharp::THETA_REF_PRESSURE);
    theta_e_arr[idx] = sharp::thetae(pres, tmpk, dwpk);

    sharp::WindComponents uv = sharp::vector_to_components(wspd, wdir);

    uwin_arr[idx] = uv.u;
    vwin_arr[idx] = uv.v;
}

// returns true if sounding loads correctly
bool read_sounding(float* pres_arr, float*hght_arr, float* tmpk_arr, 
        float* dwpk_arr, float* wdir_arr, float* wspd_arr, float* mixr_arr,
        float* vtmp_arr, float* theta_arr, float* theta_e_arr, 
        float* uwin_arr, float* vwin_arr, int* prof_N, std::string filename) {
    std::ifstream sndfile(filename);
    std::string line;

    std::string begin = "%RAW%";
    std::string end = "%END%";
    bool found_begin = false;
    bool found_end = false;
    int NLINES = 0;

    // first iteration - count the number of data
    // rows so that we can allocate arrays of appropraite
    // size to store the data
    if (sndfile.is_open()) {
        while (std::getline(sndfile, line)) {
            if (line == end) found_end = true;

            // check if we are within the data section
            if (found_begin && !found_end) {
                NLINES += 1;
            }
            // see if the data section begind
            if (line == begin) found_begin = true;
        }
    }

    // initialize profile arrays
    pres_arr = new float[NLINES];
    hght_arr = new float[NLINES];
    tmpk_arr = new float[NLINES];
    dwpk_arr = new float[NLINES];
    wdir_arr = new float[NLINES];
    wspd_arr = new float[NLINES];
    mixr_arr = new float[NLINES];
    vtmp_arr = new float[NLINES];
    theta_arr = new float[NLINES];
    theta_e_arr = new float[NLINES];
    uwin_arr = new float[NLINES];
    vwin_arr = new float[NLINES];
    *prof_N = NLINES;

    // return to the beginning of the file
    sndfile.clear();
    sndfile.seekg(0);

    found_begin = false;
    found_end = false;
    int idx = 0;
    // now loop again, this time
    // splitting the data
    if (sndfile.is_open()) {
        // iterate over the lines in the file
        while (std::getline(sndfile, line)) {
            // loop-logic: if we're at the end of the
            // data section, set that first.
            if (line == end) found_end = true;

            // check if we are within the data section
            if (found_begin && !found_end) {
                // split the line on the comma
                std::vector row = split(line, ",");

                build_profile(pres_arr, hght_arr, tmpk_arr, dwpk_arr, wdir_arr, 
                    wspd_arr, mixr_arr, vtmp_arr, theta_arr, theta_e_arr, 
                    uwin_arr, vwin_arr, prof_N, row, idx);

                idx += 1;
            }
            // see if the data section begind
            if (line == begin) found_begin = true;
        }
        sndfile.close();
        std::cout << "Success reading: " << filename << std::endl;
        std::cout << "Number of vertical levels: " << *prof_N << std::endl;

        return true;
    }

    else {
        std::cout << "Unable to open file: " << filename << std::endl;
        return false;
    }
}

int main(int argc, char* argv[]) {
    std::string snd_file1 =
        "../../data/test_snds/20160524_2302_EF3_37.57_-100.13_108_613967.snd";
    std::string snd_file2 = "../../data/test_snds/hires-SPC.txt";

    float* pres_arr; 
    float* hght_arr; 
    float* tmpk_arr;
    float* dwpk_arr;
    float* wdir_arr; 
    float* wspd_arr; 
    float* mixr_arr;
    float* vtmp_arr; 
    float* theta_arr; 
    float* theta_e_arr; 
    float* uwin_arr; 
    float* vwin_arr; 
    int* prof_N;

    bool prof = read_sounding(pres_arr, hght_arr, tmpk_arr, dwpk_arr, wdir_arr, wspd_arr, 
            mixr_arr, vtmp_arr, theta_arr, theta_e_arr, uwin_arr, vwin_arr, 
            prof_N, snd_file1);

    if (prof) {
        static constexpr sharp::lifter_wobus lifter;
        sharp::Parcel sfc_pcl;
        sharp::Parcel mu_pcl;
        sharp::Parcel ml_pcl;

        sharp::define_parcel(prof->pres, prof->tmpk, prof->dwpk, prof->mixr,
                             prof->theta, prof->theta_e, prof->NZ, sfc_pcl,
                             sharp::LPL::SFC);
        sharp::define_parcel(prof->pres, prof->tmpk, prof->dwpk, prof->mixr,
                             prof->theta, prof->theta_e, prof->NZ, ml_pcl,
                             sharp::LPL::ML);
        sharp::define_parcel(prof->pres, prof->tmpk, prof->dwpk, prof->mixr,
                             prof->theta, prof->theta_e, prof->NZ, mu_pcl,
                             sharp::LPL::MU);

        auto start_time = std::chrono::system_clock::now();

        sharp::lift_parcel(lifter, prof->pres, prof->vtmp, prof->buoyancy,
                           prof->NZ, &sfc_pcl);
        sharp::cape_cinh(prof->pres, prof->hght, prof->buoyancy, prof->NZ,
                         &sfc_pcl);

        sharp::lift_parcel(lifter, prof->pres, prof->vtmp, prof->buoyancy,
                           prof->NZ, &ml_pcl);
        sharp::cape_cinh(prof->pres, prof->hght, prof->buoyancy, prof->NZ,
                         &ml_pcl);

        sharp::lift_parcel(lifter, prof->pres, prof->vtmp, prof->buoyancy,
                           prof->NZ, &mu_pcl);
        sharp::cape_cinh(prof->pres, prof->hght, prof->buoyancy, prof->NZ,
                         &mu_pcl);

        auto end_time = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
                            end_time - start_time)
                            .count();

        std::cout << "Lifting 3 parcels took: " << duration << "us"
                  << std::endl;

        std::cout << "SFC PCL\t";
        std::cout << "LFC PRES: " << sfc_pcl.lfc_pressure << "\t";
        std::cout << "EL PRES: " << sfc_pcl.eql_pressure << "\t";
        std::cout << "CAPE: " << sfc_pcl.cape << "\t";
        std::cout << "CINH: " << sfc_pcl.cinh << std::endl;

        std::cout << "MU PCL\t";
        std::cout << "LFC PRES: " << mu_pcl.lfc_pressure << "\t";
        std::cout << "EL PRES: " << mu_pcl.eql_pressure << "\t";
        std::cout << "CAPE: " << mu_pcl.cape << "\t";
        std::cout << "CINH: " << mu_pcl.cinh << std::endl;

        std::cout << "ML PCL\t";
        std::cout << "LFC PRES: " << ml_pcl.lfc_pressure << "\t";
        std::cout << "EL PRES: " << ml_pcl.eql_pressure << "\t";
        std::cout << "CAPE: " << ml_pcl.cape << "\t";
        std::cout << "CINH: " << ml_pcl.cinh << std::endl;
    }
}
