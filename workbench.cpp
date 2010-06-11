
// Include libtorrent
#include "libtorrent/session.hpp"
#include "libtorrent/torrent_handle.hpp"
#include "libtorrent/create_torrent.hpp"
#include "libtorrent/alert_types.hpp"

// Include platform
#include <windows.h>
#include <windef.h>
#include <atlstr.h>
#include <shlobj.h>

// Include program
#include "resource.h"
#include "program.h"
#include "object.h"
#include "library.h"
#include "top.h"
#include "function.h"

// Access to global objects
extern handletop Handle;
extern areatop   Area;
extern datatop   Data;
extern statetop  State;

void StartLibrary() {
	try {

		// Make our libtorrent session object
		Handle.session = libtorrent::session(
			libtorrent::fingerprint(narrowRtoS(PROGRAM_NAME), PROGRAM_VERSION), // Program name and version
			std::pair(6881, 6999),                                              // Pick a port to listen on in this range
			0,                                                                  // Use the default network interface
			start_default_features | add_default_plugins,                       // Default features and plugins
			all_categories);                                                    // Subscribe to every category of alerts

		// Tell libtorrent to use all the plugins beyond the defaults
		Handle.session->add_extension(&libtorrent::create_metadata_plugin);    // Magnet links join swarm with just tracker and infohash
		Handle.session->add_extension(&libtorrent::create_ut_metadata_plugin); // Tracker and infohash swarm joining the uTorrent way
		Handle.session->add_extension(&libtorrent::create_ut_pex_plugin);      // Peer exchange
		Handle.session->add_extension(&libtorrent::create_smart_ban_plugin);   // Quickly block peers that send poison data



/*
		// Open the file
		std::wstring w(path);
		boost::filesystem::wpath p(w);
		boost::filesystem::ifstream f(p, std::ios_base::binary);

		// Initialize variables
		libtorrent::entry e = 0; // The bencoded DHT state information from the file
		bool b = false; // True once we've gotten information from the file

		if (!f.fail()) {
			try {

				// Read the file, parsing its contents into a libtorrent entry object
				e = libtorrent::bdecode(std::istream_iterator<char>(f), std::istream_iterator<char>());
				b = true; // Record that worked without an exception

			} catch (std::exception &e) { // No file or bad information inside, we'll just start the DHT without resume data
				log(widenPtoC(e.what())); // Log a note but keep going
			}
		}

		// Start the DHT
		if (b) Handle.session->start_dht(e); // With the file information we got
		else   Handle.session->start_dht();  // Without any information from a previous session
		*/








		//tell the session to listen, then start the dht


		//construct a session

		/*
		//load session state from settings file
		load_state();
		*/


		/*
		//start extensions
		add_extension();
		*/

		/*
		//start stuff
		start_dht()
		set_dht_settings()
		start_lsd()
		start_upnp()
		start_natpmp()
		*/



	} catch (std::exception &e) {
		log(widenPtoC(e.what()));
	} catch (...) {
		log(L"exception");
	}
}

void CloseLibrary() {

	/*
	stop_dht()
	stop_lsd()
	stop_upnp()
	stop_natpmp()

	//save resume data for all torrent_handles
	save_resume_data()

	//save session state
	save_state();

	//destruct session object
	*/



}

void AddTorrent() {

	/*
	bdencode();
	bencode();
	add_torrent();

	*/



}

void LibraryLoop() {

	/*

	//query the torrent handles for progress
	torrent_handle

	//query the session for information

	//add and remove torrents from the session while its running

	*/


}





/*

class session: public boost::noncopyable {

	session(fingerprint const &print = libtorrent::fingerprint("LT", 0, 1, 0, 0),
		int flags = start_default_features | add_default_plugins,
		int alert_mask = alert::error_notification);

	session(fingerprint const &print, std::pair<int, int>listen_port_range,
		char const* listen_interface = 0,
		int flags = start_default_features | add_default_plugins,
		int alert_mask = alert::error_notification);

	enum save_state_flags_t {

		save_settings = 0x001,
		save_dht_settings = 0x002,
		save_dht_proxy = 0x004,
		save_dht_state = 0x008,
		save_i2p_proxy = 0x010,
		save_encryption_settings = 0x020,
		save_peer_proxy = 0x040,
		save_web_proxy = 0x080,
		save_tracker_proxy = 0x100,
		save_as_map = 0x20
	};

	void load_state(lazy_entry const& e);
	void save_state(entry& e, boost::uint32_t flags) const;

	torrent_handle add_torrent(add_torrent_params const& params);
	torrent_handle add_torrent(add_torrent_params const& params, error_code& ec);

	void pause();
	void resume();
	bool is_paused() const;

	session_proxy abort();

	enum options_t {
		none = 0,
		delete_files = 1
	};

	enum session_flags_t {
		add_default_plugins = 1,
		start_default_features = 2
	};

	void remove_torrent(torrent_handle const &h, int options = none);
	torrent_handle find_torrent(sha_hash const &ih);
	std::vector<torrent_handle> get_torrents() const;

	void set_settings(session_settings const& settings);
	void set_pe_settings(pe_settings const& settings);

	void set_upload_rate_limit(int bytes_per_second);
	int upload_rate_limit() const;
	void set_download_rate_limit(int bytes_per_second);
	int download_rate_limit() const;

	void set_local_upload_rate_limit(int bytes_per_second);
	int local_upload_rate_limit() const;
	void set_local_download_rate_limit(int bytes_per_second);
	int local_download_rate_limit() const;

	void set_max_uploads(int limit);
	void set_max_connections(int limit);
	int max_connections() const;
	void set_max_half_open_connections(int limit);
	int max_half_open_connections() const;

	void set_peer_proxy(proxy_settings const& s);
	void set_web_seed_proxy(proxy_settings const& s);
	void set_tracker_proxy(proxy_settings const& s);

	proxy_settings const& peer_proxy() const;
	proxy_settings const& web_seed_proxy() const;
	proxy_settings const& tracker_proxy() const;

	int num_uploads() const;
	int num_connections() const;

	bool load_asnum_db(char const* file);
	bool load_asnum_db(wchar_t const* file);
	bool load_country_db(char const* file);
	bool load_country_db(wchar_t const* file);
	int as_for_ip(address const& adr);

	void set_ip_filter(ip_filter const& f);
	ip_filter const& get_ip_filter() const;

	session_status status() const;
	cache_status get_cache_status() const;

        bool is_listening() const;
        unsigned short listen_port() const;
        bool listen_on(
                std::pair<int, int> const& port_range
                , char const* interface = 0);

        std::auto_ptr<alert> pop_alert();
        alert const* wait_for_alert(time_duration max_wait);
        void set_alert_mask(int m);
        size_t set_alert_queue_size_limit(
                size_t queue_size_limit_);

        void add_extension(boost::function<
                boost::shared_ptr<torrent_plugin>(torrent*)> ext);

        void start_dht();
        void stop_dht();
        void set_dht_settings(
                dht_settings const& settings);
        entry dht_state() const;
        void add_dht_node(std::pair<std::string
                , int> const& node);
        void add_dht_router(std::pair<std::string
                , int> const& node);
        bool is_dht_running() const;

        void start_lsd();
        void stop_lsd();

        upnp* start_upnp();
        void stop_upnp();

        natpmp* start_natpmp();
        void stop_natpmp();
};



*/