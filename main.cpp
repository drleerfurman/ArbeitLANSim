// import lib;
// Stoopid IntelliSense and compilers
#include <iostream>
#include <chrono>
#include <windows.h>

#include <string>
#include <cmath>
#include <algorithm>
#include <cstdlib>
#include <vector>
#include <format>
#include <fstream>

// using namespace std;
// Turns out...

// There's literally no point in #define. At least const is supposed to point to one location, #define is just copy. Worried about some compilers having multiple
// same data copies. Also, there's no RAM benefits. Either way, data will be somewhere. Better make it fancy.
const auto defp = "default";
const auto lm = "Launch with any key... > ";
const auto ptm1 = "Pause with F1. Request for input with F2 during pause.";
const auto ptm2 = "Leave input empty or type 'cancel' to cancel the command. Use 'help' for command list.";
const auto enterwireconfigmsg = "Entering config for each of wires...\n";
const auto warnmetaio = "\t_ >> Warning: error finding relevant field during loading of metadata.\n";
const auto eS = "";
const auto warnmetaioautotrunc = "\t_ >> Warning: error during figuring out the auto-truncate field value during the load of metadata.\n";
const auto warnverunk = "\t_ >> Warning: version still unknown after metadata load attempt.\n";
const auto quitmsg = "Something has happened, or one of the peers stopped process and declared exit code 257 - not interested in contact. Exiting in...\n";
const int kc = 0x8000; // I hope I remembered it correctly.

// Arbeit LAN Ethernet simulator (2 peers)

// Hardcoded data:
const int exit_code = 257; const int start_code = 1;
const int max_wires = 16; const int min_wires = 1;
const int max_bandwith = 4294967296; // 4 Gib/s
const int min_bandwith = 1024; // Unreasonably low, 1 kib/s
const int max_ping = 200; // More than any straight-line copper cable on Earth can fit. More than equator circumference. Assuming no switches, relays, since this is a LAN. Near-c speed.
const int min_ping = 0; // Assumes contact connection
const float max_amp = 1.0; // Anti-UB
const float min_amp = -1.0; // Don't the UB
const float def_amp = 0.05; // More realistic

// Process data:
bool running = true; // You know.
bool graphChange = true; // So the first frame is drawn.
bool paused = false; // IListen loop stage state.
bool inputNow = false; // Help for renderer in console.

// Metadata:
std::string licensing; // If this is useful and may be released on public.
std::string guidelines; // Proper use and help.
std::string branch_name; // For various situations.
std::string branch_type; // Assume stability on this one.
int version = -1; // MAC for version.
bool autotruncate; // Anti-break for your computer's integrity.

// Global data: part 1
int wires = 2; // Connection number. All share same params. Not pairs, as they might be shared for direction IO. Note: it's not always an actual wire. Can be considered a simplification of EM waves/photons. Still applies. Can be thought of as frequency channels then.
int bandwith = 1048576; // The combination of voltage signal travel density per unit time and it's necessary 'thickness' in time
int ping = 10; // Time for ping-pong immediate response as of true voltage pulse. Milliseconds.
float timescale = 1.0; // Time warp inside** simulation. Usually for slow-down and inspection.
int simPeriod = 100; // The operation rate. Outside** the simulation.
int rdataseed = 0; // Seed for random data.
bool rdatarecursive = false; // If random data should re-seed itself with itself.
int xO = 0; // Left border of screen.
bool drawflags[16] = {false}; // Redraw requests

// There is an array of "Binary Series" elements for each wire and each of the BS has an integer value updated each clock if no
// interrupt flag was given. This way you can compute signal/voltage positions without actually storing a large array of each wire.

// Layout peer data
struct peerData
{
    uint8_t mac[6]; // MAC address. Actually not important that much.
    int portperiod; // How fast a voltage level can change for entire peer. Affects the propagation value stat integer.
    std::string protocol; // The network protocol name. Will be searched for in files and extracted for rules and guidelines. Can change and is having hierarchy like TCP/HTTP.
    std::string rawData; // A dataset available of the peer. Markdown in the integer array next.
    std::vector<int> datasections; // Sections for substring slicing. The last in a pair is inclusive.
    int behaviour; // Integer for remembering current state and goals. Usually used for more FSM approach.
    std::vector<int> behaviourram = {0, 0, 0}; // For multiple flags. More like BT approach.
};

// Binary Series layout data
struct binarySeries
{
    std::vector<int8_t> pulses; // Knowing the voltage level in case of an analog protocol and to know when pulse was 0.
    long long clock_birthtime; // When the BS was started in the port.
    bool direction; // You know.
};

struct wire
{
    float noise_amplitude = def_amp;
    int noise_seed = 0;
    std::vector<binarySeries> bs;
    bool sctn = false; // Signals contribute to noise. Will re-seed the noise.
    int noiseseed = 0; // First seed for random noise
    bool noiserecursive = false; // If noise should re-seed itself with itself.
};

struct protocol
{
    std::string name;
    std::vector<std::string> include;
    std::vector<std::string> lines;
};

// Global data: part 2
std::vector<wire> wiredata;
std::vector<protocol> protocols;

std::vector<size_t> find_all(std::string source, std::string target)
{
    int cur;
    // todo
    // Wait what is this???
}

void load_metadataIO()
{
    std::ifstream file("metadata.env"); std::string line; std::string field; std::string value; int pos;
    if (file.is_open())
    {
        while (getline(file, line))
        {
            pos = line.find('=');
            field = line.substr(0, pos);
            value = line.substr(pos);
            if (field == "Licensing") {licensing = value;}
            else if (field == "Guidelines") {guidelines = value;}
            else if (field == "Branch name") {branch_name = value;}
            else if (field == "Branch type") {branch_type = value;}
            else if (field == "Version") {version = stoi(value);}
            else if (field == "Auto-truncate")
            {
                if (value == "1") {autotruncate = true;}
                else if (value == "0") {autotruncate = false;}
                else {std::cout << warnmetaioautotrunc; autotruncate = false;}
            }
            else {std::cout << warnmetaio;}
            line = eS; field = eS; value = eS;
        }
        file.close();
    }
    if (version == -1) {std::cout << warnverunk;}
}

void prepareIO()
{
    int temp_1;
    std::cout << std::format("\t\t\t--- Arbeit LANSim v.{} ---\nLicensing: {}\nGuidelines: {}\nBranch name: {}\nBranch type: {}\nAuto-truncate: {}\n\n",
        version, licensing, guidelines, branch_name, branch_type, autotruncate);
    
    std::cout << std::format("Wire count (inc {} - inc {}): ", min_wires, max_wires); std::cin >> wires;
    if (wires > max_wires) {wires = max_wires;} else if (wires < min_wires) {wires = min_wires;}
    wiredata.reserve(wires);
    std::cout << std::format("Bandwith, b/s (inc {} - inc {}): ", min_bandwith, max_bandwith); std::cin >> bandwith;
    if (bandwith > max_bandwith) {bandwith = max_bandwith;} else if (bandwith < min_bandwith) {bandwith = min_bandwith;}
    std::cout << std::format("Ping (inc 0 - inc {})", max_ping); std::cin >> ping;
    if (ping > max_ping) {ping = max_ping;} else if (ping < min_ping) {ping = min_ping;}
    std::cout << "Random data seed (signed integer, 4B) (Use code 0 for random): "; std::cin >> rdataseed;
    if (rdataseed == 0) {srand(time(0)); rdataseed = rand();} srand(rdataseed);
    std::cout << "Random data should be recursive (0/1): "; std::cin >> rdatarecursive;
    std::cout << enterwireconfigmsg;
    for (int curwire = 0; curwire < wires; curwire++)
    {
        wiredata.push_back(wire());
        std::cout << std::format("Config for wire No.{} ...\n", curwire);
        std::cout << std::format("Noise amplitude [float, def={}<CUR>, max=|{}|]: ", def_amp, max_amp); std::cin >> temp_1; // Overflow is UB, you know.
        if (temp_1 > max_amp) {wiredata[curwire].noise_amplitude = max_amp;}
        else if (temp_1 < min_amp) {wiredata[curwire].noise_amplitude = min_amp;}
        std::cout << "Choose noise seed <signed integer>: "; std::cin >> wiredata[curwire].noise_seed;
    }
    wiredata.shrink_to_fit(); // Performance
}

void tweak_peer(peerData* peer)
{
    // todo Guess this will be anotehr 100 lines. I like how everyone's impressed at line count.
    // MAC
    // PORT
    // PROTOCOL/PROTOCOL/PROTOCOL
    // rawdata [SELECT TO DRINK FROM FILE]
    // datasections setup
    // behaviour flag setup
    // RAM behaviours
}

void fill_peer(peerData* peer)
{
    char tweak;
    std::string baked_cout = "Notice about auto-fill of a peer:\n\tMAC: ";
    for (uint8_t i = 0; i < 6; i++)
    {
        if (i == 0) {(*peer).mac[i] = rand() % 128;}
        else {(*peer).mac[i] = rand() % 256;}
        if (rdatarecursive) {srand((*peer).mac[i]);}
        baked_cout += std::format("{:02X}", (*peer).mac[i]);
        if (i != 5) {baked_cout += ':';}
    }
    (*peer).portperiod = rand() % 10; // Nanoseconds.
    (*peer).protocol = "ARBPH/ARBPING"; // todo: temporary
    (*peer).rawData = "\n\n\t\t\t--- SAMPLE DATA ---\n";
    (*peer).datasections = {0, 25};
    (*peer).behaviour = 1; // Is on, start, wishing a connection.
    (*peer).behaviourram = {1}; // ON + START + WANTS CON
    baked_cout += std::format("\n\tPort period: {} ns\n\tProtocol: {}\nIf you want to tweak these values, confirm with Y/y: ", (*peer).portperiod, (*peer).protocol);
    std::cout << baked_cout; std::cin >> tweak;
    if ((tweak == 'y') || (tweak == 'Y')) {tweak_peer(peer);}
}

void audio_processor(int wire_id, int duration, int basefreq)
{
    // todo processing active wire
    // WARNING: NO VOLUME TWEAKING!
    int8_t temp;
    // wiredata[wire_id].);
}

int8_t readwire(int wire_id, bool side)
{
    // Reads given noise. That's the fun.
    srand(wiredata[wire_id].noise_seed);
    int8_t result = static_cast<int8_t>(wiredata[wire_id].noise_amplitude * (rand() % 127) % 127); // Supposed* to be fine
    for (int i = 0; i < wiredata[wire_id].bs.size(); i++)
    {
        if (ping / 2)
    }
}

void gui_symbols()
{
    // todo
    if (drawflags[0])
    {
        // event log
        //
    }
    if (drawflags[1])
    {
        // from-A wire inspector
    }
    if (drawflags[2])
    {
        // from-B wire inspector
    }
}

void load_protocol(std::string protocolName)
{
    std::fstream protocol_file(protocolName, std::ios::in);
    std::string line; int latest_len = protocols.size();
    protocols.push_back(protocol());
    while (getline(protocol_file, line))
    {
        protocols[latest_len].lines.push_back(line);
    }
}

void default_protocol(peerData* peer)
{
    if ((*peer).behaviourram[1] == 0)
    {
        // Send ping
    }
}

void think_peer(peerData* peer)
{
    if ((*peer).protocol == defp)
    {default_protocol(peer);}
    else
    {
        // todo find and execute protocol advances
    }
    // todo reframe the timescale tick assumption
}

void tick_world()
{
    for (int i = 0; i < wiredata.size(); i++)
    {
        if (wiredata[i].noiserecursive) {srand(wiredata[i].noise_seed); wiredata[i].noise_seed = rand();} // Recursive noise seeding per wire
    }
}

void IListen()
{
    // todo more keys
    if (GetAsyncKeyState(VK_F1) & kc)
    {
        paused = true; Sleep(1000);
    }
    while (paused)
    {
        if (GetAsyncKeyState(VK_F2) & kc)
        {
            inputNow = true; gui_symbols(); std::string buffer;
            std::cin >> buffer;
            // todo
            inputNow = false; Sleep(1000);
        }
        if (GetAsyncKeyState(VK_F1) & kc)
        {
            paused = false; Sleep(1000);
        }
    }
}

double deltaClock()
{
    // todo
}

int main()
{
    peerData A; peerData* pA = &A;
    peerData B; peerData* pB = &B;

    fill_peer(pA);
    fill_peer(pB);

    long long clock = 0; // Global clock

    // WIP

    std::cout << lm; std::cin.get(); // Launch

    while (running)
    {
        Sleep(simPeriod); // Get a better clock delta method
        IListen(); // yeah it listens
        think_peer(pA); think_peer(pB); // smart
        tick_world(); // Can't know the noise until it happens.
        if ((A.behaviour == exit_code) || (B.behaviour == exit_code)) {break;}
        gui_symbols(); // draw based on changes
        clock += 1;
        // todo
    }
    std::cout << quitmsg;
    for (int i = 10; i < 0; i--)
    {
        std::cout << '\r' << i;
        Sleep(500);
    }
}