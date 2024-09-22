#pragma once
#include <wx/wx.h>
#include <wx/spinctrl.h>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <wx/taskbar.h>
#include <wx/menu.h>

class TaskBarIcon;
class MuteFrame;
class MainFrame;

class MuteFrame {
public:
	MuteFrame();
	MuteFrame(int start_year, int start_month, int start_day, int start_hour, int start_minute, int end_year, int end_month, int end_day, int end_hour, int end_minute, bool repeat_every_week);
	std::string to_string();
	bool does_overlap_with_current_time();
	
//private:
	int id;
	int start_year;
	int start_month;
	int start_day;
	int start_hour;
	int start_minute;
	int end_year;
	int end_month;
	int end_day;
	int end_hour;
	int end_minute;
	bool repeat_every_week;
};

class MainFrame : public wxFrame {
public:
	MainFrame(const wxString& title);
	void OnMenuEvent(wxCommandEvent& event);
	void delete_frame(int line_no);

private:
	wxRadioBox* start_day;
	wxSpinCtrl* start_hour;
	wxSpinCtrl* start_minute;
	wxRadioBox* end_day;
	wxSpinCtrl* end_hour;
	wxSpinCtrl* end_minute;
	wxCheckBox* repeat_every_week;
	wxButton* add_button;
	wxListBox* frame_list;
	wxButton* autostart_button;
	TaskBarIcon* task_bar_icon;

	std::condition_variable cv;
	std::mutex mtx;
	std::thread thread_event;
	bool terminate_thread = false;


	void OnAddButtonClicked(wxCommandEvent& event);
	void autostart_button_clicked(wxCommandEvent& event);
	void manage_frames_in_thread();
	int manage_frames();
	~MainFrame();
	void OnClose(wxCloseEvent& event);

};



class TaskBarIcon : public wxTaskBarIcon {
public:
	TaskBarIcon(MainFrame* parentFrame);
	void left_button_click(wxTaskBarIconEvent&);
	void right_button_click(wxTaskBarIconEvent&);

private:
	MainFrame* main_frame;
};

