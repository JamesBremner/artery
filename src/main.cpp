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
    void nodeLocate(const std::string &pointName);
    void linkLocate(const std::string &pointName);
    void linkNodes();
    void autoScan();
};

std::string cLink::text()
{
    std::stringstream ss;
    // ss << e1.first << "," << e1.second
    //    << " <-> " << e2.first << "," << e2.second
    //    << "  " << n1 << "<->" << n2;
    ss  << n1 << "<->" << n2;
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

    // myViewMenu->append(theLanguage.get("Points"), [&](const std::string &title)
    //                    {
    //     mypnlMeasure.show( false );
    //     mylbPoint.text( thePoints.text() );
    //     mylbPoint.update();
    //     mylbPoint.show(); });
    // myViewMenu->append(theLanguage.get("Measure Values"), [&](const std::string &title)
    //                    { measurePanelsUpdate(); });
    // myViewMenu->append(theLanguage.get("Measure Normal Ranges Editor"), [&](const std::string &title)
    //                    { normalRangesEdit(); });

    // myViewMenu->append(theLanguage.get("English"), [&](const std::string &title)
    //                    {
    //     theLanguage.setEnglish();
    //     myViewMenu->check( 4, true );
    //     myViewMenu->check( 5, false );
    //     myForm.text("Cephacile"); });
    // myViewMenu->append(theLanguage.get("French"), [&](const std::string &title)
    //                    {
    //     theLanguage.setFrench();
    //     myViewMenu->check( 4, false );
    //     myViewMenu->check( 5, true );
    //     myForm.text("Céphacile"); });
    // myViewMenu->append(theLanguage.get("Optional Points"), [&](const std::string &title)
    //                    { optionalPoints(); });

    // cDatabase db;
    // if (!db.isFrench())
    // {
    //     myViewMenu->check(4, true);
    //     myViewMenu->check(5, false);
    //     myForm.text("Cephacile");
    // }
    // else
    // {
    //     myViewMenu->check(4, false);
    //     myViewMenu->check(5, true);
    //     myForm.text("Céphacile");
    // }

    // /////// Database menu

    // wex::menu d(myForm);

    // d.append(theLanguage.get("Load Patient"), [&](const std::string &title)
    //          { patientLoad(); });
    // d.append(theLanguage.get("Save Patient"), [&](const std::string &title)
    //          { patientSaveExisting(); });
    // d.append(theLanguage.get("New Patient"), [&](const std::string &title)
    //          { patientAdd(); });

    // mb.append(theLanguage.get("Database"), d);
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
        { nodeLocate(title); });
    myPointMenu->append(
        "Link end point", [&](const std::string &title)
        { linkLocate(title); });
    myPointMenu->popup(myPointSelectedX, myPointSelectedY);
}

void cGUI::nodeLocate(const std::string &pointName)
{
    // locate point where mouse was when menu popped up
    float scale = 1000 / myXrayDisplayWidth;
    int x = myPointSelectedX;
    int y = myPointSelectedY;
    myNodeLocs.push_back({x, y});
    linkNodes();
    mypnlMeasure.update();
    mypnlxray.update();
}

void cGUI::linkLocate(const std::string &pointName)
{
    float scale = 1000 / myXrayDisplayWidth;
    int x = myPointSelectedX;
    int y = myPointSelectedY;
    myLinkLocs.push_back({x, y});
    linkNodes();
    mypnlMeasure.update();
    mypnlxray.update();
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

        // thePoints.draw( S, 1000 / myXrayDisplayWidth );

        // mypnlMeasure.move( maxw+50,20,700,800);

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

void cGUI::linkNodes()
{
    myLinks.clear();
    for (int kl = 0; kl < (int)myLinkLocs.size() - 1; kl += 2)
    {
        cLink link;
        auto ep = myLinkLocs[kl];
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
        link.n1 = closest;
        link.e1 = ep;

        ep = myLinkLocs[kl + 1];
        mind = MAXINT;
        kn = -1;
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
        link.n2 = closest;
        link.e2 = ep;
        myLinks.push_back(link);
    }
}

void cGUI::autoScan()
{
    myNodeLocs.clear();
    if (!myXrayFilename.length())
        return;
    Gdiplus::RectF bounds;
    Gdiplus::Unit unit;
    Gdiplus::Color color;
    myXrayBitmap->GetBounds(&bounds, &unit);
    std::cout << bounds.Width << " by " << bounds.Width << "\n";
    for (int kx = 0; kx < myXrayDisplayWidth; kx++)
    {
        std::cout << "\n";
        for (int ky = 0; ky < bounds.Height; ky++)
        {
            myXrayBitmap->GetPixel(kx, ky, &color);
            if (color.GetBlue() > 0xF0 &&
                color.GetRed() < 10 &&
                color.GetGreen() < 10)
            {
                // std::cout << kx << " " << ky << ",";
                if (std::find_if(
                        myNodeLocs.begin(), myNodeLocs.end(),
                        [&](const std::pair<int, int> &n)
                        {
                            return (fabs(n.first - kx) <= 30 &&
                                    fabs(n.second - ky) <= 30);
                        }) == myNodeLocs.end())
                {
                    myNodeLocs.push_back({kx + 15, ky});
                    // std::cout <<" nodes "<< vNode.size() << " inserted\n";
                    // for( auto& p : vNode )
                    //     std::cout << p.first <<"," << p.second << " ";
                    // std::cout << "\n\n";
                }
            }
        }
    }
    linkNodes();
    mypnlMeasure.update();
    mypnlxray.update();
}

main()
{
    cGUI theGUI;
    return 0;
}
