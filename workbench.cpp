
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


			if (info.has_data) {
				log(L"resume_found: ");
				--num_resume_data;
			}
		}

	} catch (std::exception &e) {
		log(widenPtoC(e.what()));
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


			alerts = Handle.session->pop_alert();
		}

	} catch (std::exception &e) {
		log(widenPtoC(e.what()));//TODO should be widenPtoC, i think
	} catch (...) {
		log(L"exception");
	}
}


// Given a libtorrent alert, fill a structure of info about it
// After calling this function, you can look at the information in info to see the alert libtorrent sent you
void ProcessAlert(const libtorrent::alert *alert, alert_structure *info) {

	// Get the category and the message
	info->category = alert->category();
	info->message = widenStoC(alert->message());

	// If it's a torrent alert
	const libtorrent::torrent_alert *a = dynamic_cast<const libtorrent::torrent_alert *>(alert);
	if (a) {

		// Get the torrent handle and make sure that torrent is initialized and not yet aborted
		libtorrent::torrent_handle h = a->handle;
		if (h.is_valid()) {

			// Get the infohash
			info->sha1 = HashToString(h.info_hash());

			// If the alert is for save resume data
			const libtorrent::save_resume_data_alert *a1 = dynamic_cast<const libtorrent::save_resume_data_alert *>(alert);
			if (a1) {

				// Get the pointer to the resume data
				const boost::shared_ptr<libtorrent::entry> resume_ptr = a1->resume_data;
				info->has_data = 1; // Mark that this info structure has resume data
				info->resume_data = resume_ptr.get(); // Copy across the pointer to the resume data
				return;
			}

			// If the alert is for save resume data failed
			const libtorrent::save_resume_data_failed_alert *a2 = dynamic_cast<const libtorrent::save_resume_data_failed_alert *>(alert);
			if (a2) {

				// Get the error message
				info->message = widenStoC(a2->msg);
				return;
			}

			// If the alert is for fast resume rejected
			const libtorrent::fastresume_rejected_alert *a3 = dynamic_cast<const libtorrent::fastresume_rejected_alert *>(alert);
			if (a3) {

				// Get the error message
				info->message = widenStoC(a3->msg);
				return;
			}
		}
	}
}




