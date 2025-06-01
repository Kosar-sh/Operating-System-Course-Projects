#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <mutex>
#include <sndfile.h>
#include <cmath>
#include <chrono>

using namespace std;

struct audio
{
    vector<float> data;
    int sample_rate;
    int channels;
};

audio read_wav_files(const string &filename)
{
    SF_INFO sfInfo;
    SNDFILE *inFile = sf_open(filename.c_str(), SFM_READ, &sfInfo);
    if (!inFile)
    {
        cerr << "Error opening input file: " << sf_strerror(NULL) << endl;
        exit(1);
    }
    vector<float> data(sfInfo.frames * sfInfo.channels);
    sf_read_float(inFile, data.data(), data.size());

    sf_close(inFile);

    return {data, sfInfo.samplerate, sfInfo.channels};
}

void write_wav_file(const string &filename, const audio &wav)
{
    SF_INFO sfInfo;
    sfInfo.samplerate = wav.sample_rate;
    sfInfo.channels = wav.channels;
    sfInfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;

    SNDFILE *file = sf_open(filename.c_str(), SFM_WRITE, &sfInfo);
    if (!file)
    {
        cerr << "Error: Could not open output WAV file " << filename << endl;
        exit(1);
    }

    sf_write_float(file, wav.data.data(), wav.data.size());
    sf_close(file);
}

int FIR_filter_function(vector<float> &FIR_data)
{
    vector<float> coefficients = {0.2f, 0.25f, 0.3f, 0.4f};
    vector<float> output_audio(FIR_data.size(), 0.0f);
    for (int i = 0; i < FIR_data.size(); ++i)
    {
        for (int j = 0; j < coefficients.size(); ++j)
        {
            if (i >= j)
            {
                output_audio[i] += coefficients[j] * FIR_data[i - j];
            }
        }
    }
    FIR_data = output_audio;
    return 0;
}

int apply_IIR(vector<float> &IIR_data, vector<float> &output, int Length_input,
              vector<double> iir_FF_coefficents, vector<double> iir_FB_coefficents)
{
    int size_FF = iir_FF_coefficents.size();
    int size_FB = iir_FB_coefficents.size();
    for (int n = 0; n < Length_input; ++n)
    {
        for (int k = 0; k < size_FF; ++k)
        {
            if (n >= k)
            {
                output[n] += iir_FF_coefficents[k] * IIR_data[n - k]; // Feedforward
            }
        }
        for (int j = 1; j < size_FB; ++j)
        {
            if (n >= j)
            {
                output[n] -= iir_FB_coefficents[j] * output[n - j]; // Feedback
            }
        }
    }
    return 0;
}
int IIR_filter_function(vector<float> &IIR_data)
{
    int Length_input = IIR_data.size();
    vector<float> output(Length_input, 0.0);
    vector<double> iir_FF_coefficents = {0.1, 0.2, 0.3, 0.35};
    vector<double> iir_FB_coefficents = {1.0, -0.5, 0.25, 0.3};
    if (!apply_IIR(IIR_data,output,Length_input,iir_FF_coefficents,iir_FB_coefficents))
    {
        IIR_data=output;
    }
    return 0;
}

int bandpass_filter_function(vector<float> &bandpass_data)
{
    for (auto &sample : bandpass_data)
    {
        double numerator = sample * sample;
        double denominator = (sample * sample) + (0.4 * 0.4);
        sample = (numerator / denominator);
    }
    return 0;
}

int notch_filter_function(vector<float> &notch_data)
{
    for (auto &sample : notch_data)
    {
        double division = (sample / 0.05);
        double exponent = division * division;
        sample = (1.0 / (exponent + 1.0)) * sample;
    }
    return 0;
}

void print_execution_time(chrono::time_point<chrono::high_resolution_clock> &start,
                          string name)
{
    auto end = chrono::high_resolution_clock::now();
    static auto s = start;
    chrono::duration<double, milli> elapsed = end - start;
    cout << name << " : " << elapsed.count() << " miliseconds " << endl;
    start = end;
}

void write_audios(const audio &read_audio, const audio &BandPass_audio, const audio &Notch_audio,
                  const audio &FIR_audio, const audio &IIR_audio)
{
    write_wav_file("output_BandPass_serial.wav", BandPass_audio);
    write_wav_file("output_Notch_serial.wav", Notch_audio);
    write_wav_file("output_FIR_serial.wav", FIR_audio);
    write_wav_file("output_IIR_serial.wav", IIR_audio);
}

audio apply_BandPass(const audio &read_audio)
{
    audio BandPass_audio = read_audio;
    auto start = chrono::high_resolution_clock::now();
    if (!bandpass_filter_function(BandPass_audio.data))
        print_execution_time(start, "BandPass_filter Time");
    return BandPass_audio;
}

audio apply_notch(const audio &read_audio)
{
    audio Notch_audio = read_audio;
    auto start = chrono::high_resolution_clock::now();
    if (!notch_filter_function(Notch_audio.data))
        print_execution_time(start, "Notch_filter Time");
    return Notch_audio;
}

audio apply_FIR(const audio &read_audio)
{
    audio FIR_audio = read_audio;
    auto start = chrono::high_resolution_clock::now();
    if (!FIR_filter_function(FIR_audio.data))
        print_execution_time(start, "FIR_filter Time");
    return FIR_audio;
}

audio apply_IIR(const audio &read_audio)
{
    audio IIR_audio = read_audio;
    auto start = chrono::high_resolution_clock::now();
    if (!IIR_filter_function(IIR_audio.data))
        print_execution_time(start, "IIR_filter Time");
    return IIR_audio;
}


int Apply_filters_serial(const audio &read_audio)
{
    audio BandPass_audio = apply_BandPass(read_audio); // BANDPASS;
    audio Notch_audio = apply_notch(read_audio);       // NOTCH;
    audio FIR_audio = apply_FIR(read_audio);           // FIR;
    audio IIR_audio = apply_IIR(read_audio);           // IIR;
    auto start = chrono::high_resolution_clock::now();
    write_audios(read_audio, BandPass_audio, Notch_audio, FIR_audio, IIR_audio);
    print_execution_time(start, "Write Time");
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        cout << "The input file is not provided" << endl;
        return 1;
    }

    string input_name = argv[1];
    auto start = chrono::high_resolution_clock::now();
    audio read_audio = read_wav_files(input_name);
    print_execution_time(start, "Read Time");
    if (Apply_filters_serial(read_audio))
    {
        cout << "Error In Applying Filters." << endl;
        return 1;
    }
    return 0;
}
