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
    int sampleRate;
    int channels;
};

audio read_wav_files(const string &filename)
{
    SF_INFO sfInfo;
    SNDFILE *file = sf_open(filename.c_str(), SFM_READ, &sfInfo);
    if (!file)
    {
        cerr << "Error: Could not open input WAV file " << filename << endl;
        exit(1);
    }

    vector<float> data(sfInfo.frames * sfInfo.channels);
    sf_read_float(file, data.data(), data.size());
    sf_close(file);
    return {data, sfInfo.samplerate, sfInfo.channels};
}

void write_wav_file(const string &filename, const audio &wav)
{
    SF_INFO sfInfo;
    sfInfo.samplerate = wav.sampleRate;
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

void set_FIR_partition(vector<float> &fir_data, vector<float> &output, int start, int end)
{
    vector<float> coefficients = {0.2f, 0.25f, 0.3f, 0.4f};
    int numb_element = 0;
    for (int i = start; i < end; ++i)
    {
        float result = 0.0f;
        for (int j = 0; j < coefficients.size(); ++j)
        {
            if (i >= j)
            {
                result += coefficients[j] * fir_data[i - j];
            }
        }
        output[i] = result;
    }
    numb_element = end;
}

int FIR_parallel_function(vector<float> &fir_data)
{
    int count_thread = thread::hardware_concurrency();
    int partition_size = fir_data.size() / count_thread;
    vector<float> output(fir_data.size(), 0.0f);
    vector<thread> threads;
    for (int i = 0; i < count_thread; ++i)
    {
        int start = i * partition_size;
        int end = (i == count_thread - 1) ? fir_data.size() : (i + 1) * partition_size;
        threads.emplace_back(set_FIR_partition, ref(fir_data), ref(output), start, end);
    }
    for (auto &t : threads)
    {
        t.join();
    }
    fir_data = output;
    return 0;
}

void set_IIR_FF_partitions(vector<float> &irr_data, vector<float> &output,
                           int start, int end, const vector<double> &feedforward)
{
    int FF_elements = feedforward.size();
    for (int n = start; n < end; ++n)
    {
        for (int k = 0; k < FF_elements; ++k)
        {
            if (n >= k)
            {
                output[n] += feedforward[k] * irr_data[n - k];
            }
        }
    }
}

int make_IIR_threads(vector<float> &iir_data, vector<float> &output, int &length_input,
                     vector<double> &iir_FF_coefficents)
{
    vector<thread> threads;
    int count_thread = thread::hardware_concurrency();
    int partition_size = length_input / count_thread;
    int start = 0;
    for (int i = 0; i < count_thread; ++i) // FeedForward
    {
        int end = (i == count_thread - 1) ? length_input : start + partition_size;
        threads.emplace_back(set_IIR_FF_partitions, ref(iir_data), ref(output), start, end, ref(iir_FF_coefficents));
        start = end;
    }
    for (auto &t : threads)
    {
        t.join();
    }
    return 0;
}

int IIR_parallel_function(vector<float> &iir_data)
{
    vector<double> iir_FF_coefficents = {0.1, 0.2, 0.3, 0.35};
    vector<double> iir_FB_coefficents = {1.0, -0.5, 0.25, 0.3};
    int length_input = iir_data.size();
    vector<float> output(length_input, 0.0f);
    make_IIR_threads(iir_data, output, length_input, iir_FF_coefficents);
    for (int n = 0; n < length_input; ++n) // FeedBackward
    {
        for (int j = 1; j < iir_FB_coefficents.size(); ++j)
        {
            if (n >= j)
            {
                output[n] -= iir_FB_coefficents[j] * output[n - j];
            }
        }
    }
    iir_data = output;
    return 0;
}

void set_BandPass_partition(vector<float> &band_data, int start, int end)
{
    for (int i = start; i < end; ++i)
    {
        double numerator = band_data[i] * band_data[i];
        double denominator = (band_data[i] * band_data[i]) + (0.4 * 0.4);
        band_data[i] = numerator / denominator;
    }
}

int BandPass_parallel_func(vector<float> &band_data)
{
    int count_thread = thread::hardware_concurrency();
    int partition_size = band_data.size() / count_thread;
    vector<thread> threads;
    for (int i = 0; i < count_thread; ++i)
    {
        int start = i * partition_size;
        int end = (i == count_thread - 1) ? band_data.size() : (i + 1) * partition_size;
        threads.emplace_back(set_BandPass_partition, ref(band_data), start, end);
    }
    for (auto &t : threads)
    {
        t.join();
    }
    return 0;
}

void set_notch_partitions(vector<float> &notch_data, int start, int end)
{
    for (int i = start; i < end; ++i)
    {
        double division = (notch_data[i] / 0.06);
        double exponent = division * division;
        notch_data[i] = (1.0 / (exponent + 1.0)) * notch_data[i];
    }
}

int notch_parallel_func(vector<float> &signal)
{
    int thread_count = thread::hardware_concurrency();
    int partition_size = signal.size() / thread_count;
    vector<thread> threads;
    for (int i = 0; i < thread_count; ++i)
    {
        int start = i * partition_size;
        int end = (i == thread_count - 1) ? signal.size() : (i + 1) * partition_size;
        threads.emplace_back(set_notch_partitions, ref(signal), start, end);
    }
    for (auto &t : threads)
    {
        t.join();
    }
    return 0;
}

void print_execution_time(chrono::time_point<chrono::high_resolution_clock> &start, string name)
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
    vector<thread> threads;
    threads.emplace_back([&]
                         { write_wav_file("output_BandPass_Parallel.wav", BandPass_audio); });
    threads.emplace_back([&]
                         { write_wav_file("output_Notch_Parallel.wav", Notch_audio); });
    threads.emplace_back([&]
                         { write_wav_file("output_FIR_Parallel.wav", FIR_audio); });
    threads.emplace_back([&]
                         { write_wav_file("output_IIR_Parallel.wav", IIR_audio); });

    for (auto &t : threads)
    {
        t.join();
    }
}

audio apply_BandPass(const audio &read_audio)
{
    audio BandPass_audio = read_audio;
    auto start = chrono::high_resolution_clock::now();
    BandPass_parallel_func(BandPass_audio.data);
    print_execution_time(start, "BandPass_filter Time");
    return BandPass_audio;
}

audio apply_notch(const audio &read_audio)
{
    audio Notch_audio = read_audio;
    auto start = chrono::high_resolution_clock::now();
    notch_parallel_func(Notch_audio.data);
    print_execution_time(start, "Notch_filter Time");
    return Notch_audio;
}

audio apply_FIR(const audio &read_audio)
{
    audio FIR_audio = read_audio;
    auto start = chrono::high_resolution_clock::now();
    FIR_parallel_function(FIR_audio.data);
    print_execution_time(start, "FIR_filter Time");
    return FIR_audio;
}

audio apply_IIR(const audio &read_audio)
{
    audio IIR_audio = read_audio;
    auto start = chrono::high_resolution_clock::now();
    IIR_parallel_function(IIR_audio.data);
    print_execution_time(start, "IIR_filter Time");
    return IIR_audio;
}

int Apply_filters_parallel(const audio &read_audio)
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
    if (Apply_filters_parallel(read_audio))
    {
        cout << "Error In Applying Filters." << endl;
        return 1;
    }
    return 0;
}
