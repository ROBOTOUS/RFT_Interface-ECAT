#include "RT_CommonUtility.h"

#define _USE_MATH_DEFINES // 상수 사용하기 위해
#include <math.h>

/***************************************************************/
/*	File Handling												/
/***************************************************************/

bool get_attributes_from_filename(CString Filename, FILENAME_ATTRIBUTES& fna)
{
	int fn_length = Filename.GetLength();
	if (fn_length == 0)
		return false;
	int	pos;

	// find position of '\'
	pos = Filename.ReverseFind(_T('\\'));
	if (pos == -1) // no path
	{
		fna.path = _T("");
		fna.filename = Filename;
	}
	else
	{
		if ( pos == (fn_length-1) ) // 파일명이 \로 끝나는 경우
		{
			fna.path = Filename.Left(pos);	
			fna.filename = fna.filename_no_ext = fna.extension = fna.tag = _T(""); // path만 채워서 리턴
			return true;
		}
		else
		{
			fna.path = Filename.Left(pos);
			fna.filename = Filename.Right(fn_length - pos - 1);
		}
	}

	// find position of '.'
	pos = fna.filename.ReverseFind(_T('.'));
	if (pos == -1) // no extension
	{
		fna.filename_no_ext = fna.filename;
		fna.extension = _T("");
	}
	else
	{
		fna.filename_no_ext = fna.filename.Left(pos);
		fna.extension = fna.filename.Right(fna.filename.GetLength() - pos - 1);
	}

	// find position of '_' 
	pos = fna.filename_no_ext.Find(_T('_'));  // find first tag position
	if (pos == -1) // no tag
	{
		fna.tag = _T("");
	}
	else
	{
		fna.tag = fna.filename_no_ext.Right(fna.filename_no_ext.GetLength() - pos); // '_' 태그 기호 포함
	}

	return true;
}

bool make_folder_if_not_exist(CString folder, CString *return_msg)
{
	bool ret = true;
	CString fn = folder;
	if (fn.GetAt(folder.GetLength() - 1) == _T('\\'))  // \로 끝나는 폴더명이면 마지막 \ 삭제
		fn = folder.Left(folder.GetLength() - 1);
	if (fn.GetLength() == 0)
	{
		if (return_msg)
			*return_msg = _T("[") + folder + _T("]\n 위의 폴더명이 잘못 되었습니다.");
		return false;
	}
	std::vector<CString> not_existing_folders;
	// check if the folders in the given folder-string exist, then extract the not-exsiting folder name.
	bool stop = false;
	bool format_is_valid = true;
	do{
		if (GetFileAttributes(fn) == -1)
		{
			// save the not-existing folder to stack
			not_existing_folders.push_back(fn);
			// get the upper folder name 
			int pos = fn.ReverseFind(_T('\\'));
			if (pos == fn.GetLength() - 2) // 마지막 \의 바로 앞이 또 \일 경우
			{
				format_is_valid = false;
			}
			else
			{
				if (pos > 0)
					fn = fn.Left(pos);
				else if (pos == 0)				// 폴더명의 첫자가 \ 인 경우
					format_is_valid = false;
				else							// 더 이상 존재하지않는 상위 폴더가 없고 현재 추출된 폴더도 존재하진 않는 경우  
					format_is_valid = false;	// 최소한 드라이브의 root "drive:\" 는 존재해야함
			}
			// 마지막 드라이브명 삭제
			if (stop)
			{
				if (fn.GetAt(fn.GetLength() - 1) == _T(':'))	// 정상적인 드라이브명이면
					not_existing_folders.pop_back();			// 마지막 저장명 삭제
				else
					format_is_valid = false;
			}

			if (!format_is_valid)
			{
				if (return_msg)
					*return_msg = "[" + folder + "]\n 위의 폴더명 형식이 잘못 되었습니다.";
				return false;
			}
		}
		else
		{
			stop = true;
		}
	} while (!stop);

	int num_of_folders_created = not_existing_folders.size();

	if (num_of_folders_created > 0) // 생성해야할 폴더가 있으면
	{
		CString folders_created = _T("");
		for (int i = 0; i < num_of_folders_created; i++)
		{
			if (CreateDirectory(not_existing_folders[num_of_folders_created - i - 1], NULL)) // 마지막이 없는 폴더의 최상위이기 때문에 마지막부터 만들어야 함
			{
				folders_created += +_T("\n[") + not_existing_folders[num_of_folders_created - i - 1] + _T("]");
			}
			else
			{
				if (return_msg)
					*return_msg = _T("[") + folder + _T("]\n위의 폴더가 존재하지 않아 생성을 시도하였으나 실패하였습니다.");
				return false;
			}
		}
		if (return_msg)
			*return_msg = _T("다음의 폴더가 존재하지않아 생성됨") + folders_created ;
		return true;
	}
	else
	{
		if (return_msg)
			*return_msg = _T("");
		return true;
	}
}
/***************************************************************/
/*	Image Handling												/
/***************************************************************/

//bool save_window_item_as_image(CWnd *pWnd, int nID, CString filename, const GUID &filetype)
//{
//	CRect rect;
//	pWnd->GetDlgItem(nID)->GetClientRect(rect);
//	return save_window_item_as_image(pWnd, nID, rect, filename, filetype);
//}
//
//bool save_window_item_as_image(CWnd *pWnd, int nID, CRect rect_save, CString filename, const GUID &filetype)
//{
//	// 클라이언트 이미지 가져오기
//	CClientDC dc(pWnd);
//	CRect rect;		// the window rect of item
//
//	CImage item_image;
//
//	pWnd->GetDlgItem(nID)->GetWindowRect(&rect);	// GetClientRect을 사용하면 안된다. GetClientRect는 클라이언트 영역에 대한 위치 정보가 없다.
//	pWnd->ScreenToClient(&rect);				// 스크린에 대한 rect 정보를 현재 윈도우의 클라이언트 영역에 대한 정보로 바꿔준다.
//
//	// save할 영역이 혹시 잘못 지정될 경우에 대비하여 아이템 영역으로 한정 짓는다.
//	saturate(rect_save.left, 0L,  (LONG) rect.Width());
//	saturate(rect_save.right, 0L, (LONG)rect.Width());
//	saturate(rect_save.top, 0L, (LONG)rect.Height());
//	saturate(rect_save.bottom, 0L, (LONG)rect.Height());
//	rect_save.NormalizeRect();
//
//	item_image.Create(rect_save.Width(), rect_save.Height(), dc.GetDeviceCaps(BITSPIXEL));
//	CDC* pDC = CDC::FromHandle(item_image.GetDC()); // 저장할 이미지의 dc;
//	pDC->BitBlt(0, 0, rect_save.Width(), rect_save.Height(), &dc,
//		rect.left + rect_save.left, rect.top + rect_save.top, SRCCOPY);
//	item_image.ReleaseDC();
//
//	// image 저장 
//	if (item_image.Save(filename, filetype) == S_OK)
//		return true;
//	else
//		return false;
//}

// save the client area of a dialog window to the given format file
bool save_window_client_as_image(CWnd *pWnd, CString filename, const GUID &filetype)
{
	CRect rect;
	pWnd->GetClientRect(rect);
	return save_window_client_as_image(pWnd, rect, filename, filetype);
}

bool save_window_client_as_image(CWnd *pWnd, CRect rect_save, CString filename, const GUID &filetype)
{
	// 클라이언트 이미지 가져오기
	CClientDC dc(pWnd);
	CRect rect;		// the rect of item
	CImage client_image;

	pWnd->GetClientRect(rect); // GetClientRect을 사용하면 안된다. GetClientRect는 클라이언트 영역에 대한 위치 정보가 없다.

	// save할 영역이 혹시 잘못 지정될 경우에 대비하여 아이템 영역으로 한정 짓는다.
	saturate(rect_save.left, 0L, (LONG)rect.Width());
	saturate(rect_save.right, 0L, (LONG)rect.Width());
	saturate(rect_save.top, 0L, (LONG)rect.Height());
	saturate(rect_save.bottom, 0L, (LONG)rect.Height());
	rect_save.NormalizeRect();

	client_image.Create(rect_save.Width(), rect_save.Height(), dc.GetDeviceCaps(BITSPIXEL));
	CDC* pDC = CDC::FromHandle(client_image.GetDC()); // 저장할 이미지의 dc;
	pDC->BitBlt(0, 0, rect_save.Width(), rect_save.Height(), &dc, 
		rect.left + rect_save.left, rect.top + rect_save.top, SRCCOPY);
	client_image.ReleaseDC();

	// image 저장 
	if (client_image.Save(filename, filetype) == S_OK)
		return true;
	else
		return false;
}
/***************************************************************/
/*	Convert Angle Unit											/
/***************************************************************/

float  deg2rad(float  deg) {return (float)(deg*M_PI / 180.0);}
double deg2rad(double deg) {return ( deg*M_PI / 180.0 );}
float  rad2deg(float  rad) {return (float)( rad*180.0 / M_PI );}
double rad2deg(double rad) {return ( rad*180.0 / M_PI );}

/***************************************************************/
/*	Math.Statistics												/
/***************************************************************/

/*  get_matrix_mean()
*	For an (n x m)matrix,
*	calculate the mean(average)s of the column vectors.
*	return (m x 1) column vector
*/
VectorXd get_matrix_mean(MatrixXd& mat)
{
	return mat.colwise().mean().transpose();
	
}
VectorXf get_matrix_mean(MatrixXf& mat)
{
	return mat.colwise().mean().transpose();
}

/*  get_mean()
*
*	For a floating-point array,
*	return mean(average) value
*/
double get_mean(double *dp, int N)
{
	if (N < 1)
		return *dp; // error
	double sum = dp[0];
	for (int i = 1; i < N; i++)
	{
		sum += dp[i];
	}
	return sum / (double)N;
}
float  get_mean(float *dp, int N)
{
	if (N < 1)
		return *dp; // error
	float sum = dp[0];
	for (int i = 1; i < N; i++)
	{
		sum += dp[i];
	}
	return sum / (float)N;
}


/*  get_matrix_stdev()
*
*   For an (n x m)matrix,
*	calculate the standard deviation(s) of the column vectors.
*	return (m x 1) column vector
*/
VectorXd get_matrix_stdev(MatrixXd& mat)
{
	int col_size = mat.cols();
	VectorXd stdev_vec(col_size);

	for (int c = 0; c < col_size; c++)
	{
		stdev_vec(c) = sqrt((mat.col(c).array() - mat.col(c).mean()).square().mean());
	}
	return stdev_vec;
}
VectorXf get_matrix_stdev(MatrixXf& mat)
{
	int col_size = mat.cols();
	VectorXf stdev_vec(col_size);

	for (int c = 0; c < col_size; c++)
	{
		stdev_vec(c) = sqrt((mat.col(c).array() - mat.col(c).mean()).square().sum());
	}
	return stdev_vec;
}