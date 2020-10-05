
// PathFindingSimulatorDlg.h : fichier d'en-tête
//

#pragma once
#include <algorithm>
#include <vector>
#include <chrono>
#include <cmath>

// boîte de dialogue de CPathFindingSimulatorDlg
class CPathFindingSimulatorDlg : public CDialogEx
{
// Construction
public:
	CPathFindingSimulatorDlg(CWnd* pParent = nullptr);	// constructeur standard

// Données de boîte de dialogue
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_PATHFINDINGSIMULATOR_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// Prise en charge de DDX/DDV


// Implémentation
protected:

	struct POINT {
		float x;
		float y;
	};

	struct MOVING_UNIT {
		POINT position;
		POINT destination;
		bool moving;
		bool selected;
		float radius;
	};

	struct MOVING_GROUP {
		std::vector<MOVING_UNIT*> units;
		POINT destination_center;
		int destination_scale;
		float largest_radius;
		bool all_inside;
	};

	HICON m_hIcon;

	// Fonctions générées de la table des messages
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
	void Draw();
	void SetupUnits();
	void SelectUnits();

	CPoint mouse_selection_origin;
	CPoint mouse_selection_destination;
	bool mouse_selecting = false;
	float move_speed = 0.1f;
	std::chrono::system_clock::time_point last_frame_time;

	std::vector<MOVING_UNIT> units;
	std::vector<MOVING_GROUP> moving_groups;

public:

	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
};
