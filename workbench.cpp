
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





// Ported from http://www.rasterbar.com/products/libtorrent/manual.html#save-resume-data
void FreezeAndSaveAllFastResumeData() {
	try {

		int num_resume_data = 0;

		std::vector<libtorrent::torrent_handle> handles = Handle.session->get_torrents();
		Handle.session->pause();

		for (std::vector<libtorrent::torrent_handle>::iterator i = handles.begin(); i != handles.end(); ++i) {
			libtorrent::torrent_handle &h = *i;
			if (!h.has_metadata())
				continue;
			if (!h.is_valid())
				continue;

			h.save_resume_data();
			++num_resume_data;
			log(make(L"num_resume: ", numerals(num_resume_data)));
		}

		while (num_resume_data > 0) {
			log(make(L"waiting for resume: ", numerals(num_resume_data)));

			const libtorrent::alert *alert = Handle.session->wait_for_alert(libtorrent::seconds(10));

			// if we don't get an alert within 10 seconds, abort
			if (alert == NULL)
				break;

			std::auto_ptr<libtorrent::alert> holder = Handle.session->pop_alert();

			alert_structure info;
			ProcessAlert(alert, &info);

			//TODO This is where you look at info

			if (info.has_data) {
				log(L"resume_found: ");
				--num_resume_data;
			}
		}

	} catch (std::exception &e) {
		log(widenStoC(e.what()));
	} catch (...) {
		log(L"exception");
	}
}


void GetAlerts() {
	try {

		std::auto_ptr<libtorrent::alert> alerts;

		alerts = Handle.session->pop_alert();

		while (alerts.get()) {
			libtorrent::alert *alert = alerts.get();

			alert_structure info;

			ProcessAlert(alert, &info);

			//TODO This is where you look at info

			alerts = Handle.session->pop_alert();
		}

	} catch (std::exception &e) {
		log(widenStoC(e.what()));
	} catch (...) {
		log(L"exception");
	}
}




void ProcessAlert(const libtorrent::alert *alert, alert_structure *info) {

	info->category = alert->category();
	info->message = widenStoC(alert->message());

	const libtorrent::torrent_alert *torrentAlert;

	if ((torrentAlert = dynamic_cast<const libtorrent::torrent_alert *>(alert))) {

		libtorrent::torrent_handle handle = torrentAlert->handle;

		if (handle.is_valid()) {
			info->sha1 = HashToString(handle.info_hash());

			const libtorrent::save_resume_data_alert *srd_alert = dynamic_cast<const libtorrent::save_resume_data_alert *>(alert);
			if (srd_alert) {

				const boost::shared_ptr<libtorrent::entry> resume_ptr = srd_alert->resume_data;
				info->has_data = 1;
				info->resume_data = resume_ptr.get();
				return;
			}

			const libtorrent::save_resume_data_failed_alert *srdf_alert = dynamic_cast<const libtorrent::save_resume_data_failed_alert *>(alert);
			if (srdf_alert) {

				log(make(L"save_resume_data_failed_alert (", widenStoC(srdf_alert->msg), L")"));
				info->message = widenStoC(srdf_alert->msg);
				return;
			}

			const libtorrent::fastresume_rejected_alert *fra_alert = dynamic_cast<const libtorrent::fastresume_rejected_alert *> (alert);
			if (fra_alert) {

				log(make(L"fastresume_rejected_alert (", widenStoC(fra_alert->msg), L")"));
				info->message = widenStoC(fra_alert->msg);
				return;
			}
		}
	}
}




