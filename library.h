
// Information about the status of a torrent
struct status_structure {

	long long total_done;
	long long total_wanted_done;
	long long total_wanted;
	long long total_download;
	long long total_upload;
	long long total_payload_download;
	long long total_payload_upload;
	long long all_time_payload_download;
	long long all_time_payload_upload;
	float download_rate;
	float upload_rate;
	float download_payload_rate;
	float upload_payload_rate;
	int num_peers;
	int num_uploads;
	int num_seeds;
	int num_connections;
	int state;
	float progress;
	int paused;
	int finished;
	int valid;
	int auto_managed;
	int seeding_time;
	int active_time;
	CString error;
	CString current_tracker;
	int num_complete;
	int num_incomplete;
	long long total_failed_bytes;
};

// Information about a tracker or web seed, like the URL
struct announce_structure {

	CString url;
	int tier;
};

// Information about a torrent
struct torrent_structure {

	CString sha1;
	long long total_size;
	int piece_length;
	std::vector<announce_structure> trackers;
	std::vector<announce_structure> seeds;
	CString created_by;
	CString comment;
};

// Holds libtorrent settings
struct settings_structure {

	int max_upload_bandwidth;
	int max_download_bandwidth;
	int listen_start_port;
	int listen_end_port;
	float seed_ratio_limit;
	float seed_time_ratio_limit;
	int seed_time_limit;
	int active_downloads_limit;
	int active_seeds_limit;
	int active_limit;
	int alert_mask;
	CString listen_interface;
};

// Information about a file in a torrent
struct file_structure {

	int index;
	CString path;
	long long size;
	long long total_done;
	int priority;
};

// Information about a libtorrent alert message it has given us
struct alert_structure {

	int category;
	CString sha1;
	CString message;
	bool has_data;
	libtorrent::entry *resume_data;

	alert_structure() {
		category = -1;
		has_data = 0;
		resume_data = NULL;
	}
};

// Information about a peer in a torrent's swarm
struct peer_structure {

	int status_flags;
	CString peer_id;
	CString ip;
	int source;
	float up_speed;
	float down_speed;
	float payload_up_speed;
	float payload_down_speed;
	float progress;
	CString country;
	CString client_name;
};

// A torrent's striped pattern of pieces and their status
struct pieces_structure {

	int completed;
	CString pieces;
};
