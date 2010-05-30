



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

struct announce_structure {

	const char *url; // String
	int tier;

	~announce_structure() {
		delete[] url;
	}
};

struct torrent_structure {

	CString sha1;
	long long total_size;
	int piece_length;
	announce_structure *trackers;
	int num_trackers;
	announce_structure *seeds;
	int num_seeds;
	CString created_by;
	CString comment;
};

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
	char *listen_interface; // String
};

struct file_structure {

	int index;
	wchar_t *path; // String
	long long size;
	long long total_done;
	int priority;
};

struct alert_structure {

	int category;
	const char *sha1; // String
	const char *message; // String
	bool has_data;
	libtorrent::entry *resume_data;

	alert_structure() {
		category = -1;
		sha1 = NULL;
		message = NULL;
		has_data = 0;
		resume_data = NULL;
	}
};

struct peer_structure {

	int status_flags;
	const char *peer_id; // String
	const char *ip; // String
	int source;
	float up_speed;
	float down_speed;
	float payload_up_speed;
	float payload_down_speed;
	float progress;
	char country[3];
	const wchar_t *client_name; // String
};

struct pieces_structure {

	int completed;
	char *pieces; // String
};


