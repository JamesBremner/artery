#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <set>
#include <algorithm>
#include <windows.h>
#include <gdiplus.h>
#include <wex.h>
#include "cStarterGUI.h"

class cLink
{
public:
    std::pair<int, int> e1;
    std::pair<int, int> e2;
    int n1;
    int n2;
    std::string text();
};
class cGUI : public cStarterGUI
{
public:
    cGUI();

private:
    wex::label &lb;
    wex::panel &mypnlxray;
    wex::panel &mypnlMeasure;
    wex::menu *myViewMenu;
    wex::menu *myPointMenu;
    Gdiplus::Bitmap *myXrayBitmap;
    std::wstring myXrayFilenameW;
    std::string myXrayFilename;
    float myXrayDisplayWidth;
    int myPointSelectedX;
    int myPointSelectedY;
    std::vector<std::pair<int, int>> myNodeLocs;
    std::vector<std::pair<int, int>> myLinkLocs;
    std::vector<cLink> myLinks;

    void draw(PAINTSTRUCT &ps);
    void drawMeasure(PAINTSTRUCT &ps);
    void menus();
    void clickRight();
    void linkNodes();
    int linkNode(int epIndex);
};

std::string cLink::text()
{
    std::stringstream ss;
    // ss << e1.first << "," << e1.second
    //    << " <-> " << e2.first << "," << e2.second
    //    << "  " << n1 << "<->" << n2;
    ss << n1 << "<->" << n2;
    return ss.str();
}

cGUI::cGUI()
    : cStarterGUI(
          "Angie",
          {60, 50, 1860, 900}),
      lb(wex::maker::make<wex::label>(fm)), mypnlxray(wex::maker::make<wex::panel>(fm)), mypnlMeasure(wex::maker::make<wex::panel>(fm)),
      myPointMenu(0)
{
    lb.move(50, 50, 100, 30);

    // initialize graphics
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    // xray image
    mypnlxray.move(0, 0, 1100, 825);
    mypnlxray.nobgerase();
    mypnlxray.cursor(IDC_CROSS);

    mypnlMeasure.move(1150, 20, 700, 800);

    menus();

    mypnlxray.events().draw(
        [this](PAINTSTRUCT &ps)
        { draw(ps); });
    mypnlxray.events().clickRight(
        [&]
        { clickRight(); });

    mypnlMeasure.events().draw(
        [this](PAINTSTRUCT &ps)
        { drawMeasure(ps); });

    show();
    run();
}

void cGUI::menus()
{
    wex::menubar mb(fm);

    //////// View Menu

    myViewMenu = new wex::menu(fm);

    myViewMenu->append(
        "X-ray",
        [&](const std::string &title)
        {
                           // prompt user for X-ray file to display
                           wex::filebox fb(fm);
                           fb.open();

                           std::string sfn = fb.path();
                           myXrayFilename = sfn;
                           std::wstring ws(sfn.size(), L' ');                         // Overestimate number of code points.
                           ws.resize(std::mbstowcs(&ws[0], sfn.c_str(), sfn.size())); // Shrink to fit.
                           myXrayFilenameW = ws;

                           myXrayBitmap = new Gdiplus::Bitmap(ws.c_str());

                           //thePoints.clear();

                           //measurePanelsUpdate();

                           fm.update(); });

    // myViewMenu->append(
    //     "Auto Scan",
    //     [&](const std::string &title)
    //     {
    //         autoScan();
    //     });

    mb.append("Menu", *myViewMenu);
}

void cGUI::clickRight()
{
    auto mouse = fm.getMouseStatus();
    myPointSelectedX = mouse.x;
    myPointSelectedY = mouse.y;

    ///////// point popup menu

    delete myPointMenu;
    myPointMenu = new wex::menu(fm);
    myPointMenu->append(
        "Node", [&](const std::string &title)
        { 
            myNodeLocs.push_back({
                mouse.x,
                mouse.y});
            linkNodes();
    mypnlMeasure.update();
    mypnlxray.update(); });

    myPointMenu->append(
        "Link end point", [&](const std::string &title)
        { 
            myLinkLocs.push_back({
                mouse.x,
                mouse.y});
            linkNodes();
    mypnlMeasure.update();
    mypnlxray.update(); });

    myPointMenu->popup(myPointSelectedX, myPointSelectedY);
}

void cGUI::draw(PAINTSTRUCT &ps)
{
    wex::shapes S(ps);
    if (myXrayFilename.length())
    {
        auto myXrayBitmap = new Gdiplus::Bitmap(myXrayFilenameW.c_str());

        // window dimensions
        RECT rect;
        GetClientRect(fm.handle(), &rect);
        int rw = rect.right - rect.left;
        int rh = rect.bottom - rect.top;

        // xray display dimensions
        float maxh;
        float maxw;
        if (rw > 1100 + 700)
        {
            // large app window width
            // show xray image at a good fixed resolution
            maxw = 1100;
        }
        else if (rw > 600 + 700)
        {
            // medium app window width
            // shrink image to fit
            maxw = rw - 700;
        }
        else
        {
            // small app window width
            // keep minima image resolution by sacrificing measures description
            maxw = 600;
        }

        if (rh > 825)
            maxh = 825;
        else
            maxh = rh;

        // image dimensions
        float xh = myXrayBitmap->GetHeight();
        myXrayDisplayWidth = myXrayBitmap->GetWidth();

        // check if shrinking needed
        if (xh > maxh || myXrayDisplayWidth > maxw)
        {

            // preserve aspect ratio by scaling both dimensions by the largest required by either
            float sh = xh / maxh;
            float sw = myXrayDisplayWidth / maxw;
            float s = sh;
            if (sw > sh)
                s = sw;
            xh /= s;
            myXrayDisplayWidth /= s;
        }
        if (xh < maxh)
        {
            float s = xh / maxh;
            xh /= s;
            myXrayDisplayWidth /= s;
        }
        Gdiplus::PointF dst[] =
            {
                Gdiplus::PointF(0.0f, 0.0f),
                Gdiplus::PointF(myXrayDisplayWidth, 0.0f),
                Gdiplus::PointF(0.0f, xh),
            };

        Gdiplus::Graphics graphics(ps.hdc);
        graphics.SetCompositingMode(Gdiplus::CompositingModeSourceCopy);
        graphics.DrawImage(myXrayBitmap, dst, 3);

        delete myXrayBitmap;

        S.color(0x00FFFF);
        S.fill();
        for (auto &p : myNodeLocs)
        {
            S.circle(
                p.first,
                p.second,
                5);
        }

        S.color(0xFF0000);
        S.penThick(2);
        for (auto &l : myLinks)
        {
            S.line({l.e1.first, l.e1.second,
                    l.e2.first, l.e2.second});
        }

        SetFocus(mypnlxray.handle());
    }
    else
    {
        // S.text(theLanguage.get("Please select xray image to view"), {100,100});
        // S.text(theLanguage.get("Use menu items View / X-ray or database / load"), {100,150});
    }
}
void cGUI::drawMeasure(PAINTSTRUCT &ps)
{
    wex::shapes S(ps);
    S.text("Nodes", {20, 20, 100, 30});
    int y = 60;
    int kn = 0;
    for (auto &p : myNodeLocs)
    {
        S.text(
            std::to_string(kn++) + ": " + std::to_string(p.first) + ", " + std::to_string(p.second),
            {20, y, 100, 25});
        y += 27;
    }

    S.text("Links", {150, 20, 100, 30});
    y = 60;
    for (auto &l : myLinks)
    {
        S.text(
            l.text(),
            {150, y, 200, 25});
        y += 30;
    }
}

int cGUI::linkNode(int epIndex)
{
    auto ep = myLinkLocs[epIndex];
    int mind = MAXINT;
    int closest;
    int kn = -1;
    for (auto &n : myNodeLocs)
    {
        kn++;
        int dx = ep.first - n.first;
        int dy = ep.second - n.second;
        int d2 = dx * dx + dy * dy;
        if (d2 < mind)
        {
            mind = d2;
            closest = kn;
        }
    }
    return closest;
}

void cGUI::linkNodes()
{
    myLinks.clear();
    for (int kl = 0; kl < (int)myLinkLocs.size() - 1; kl += 2)
    {
        cLink link;
 
        link.n1 = linkNode(kl);
        link.e1 = myLinkLocs[kl];

        link.n2 = linkNode(kl+1);
        link.e2 = myLinkLocs[kl+1];;

        myLinks.push_back(link);
    }
}

main()
{
    cGUI theGUI;
    return 0;
}
