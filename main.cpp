// TODO
// Y: dodge
// a attack, b grab
#include <SFML/Graphics.hpp>
#include <SFML/System/Vector2.hpp>
#include <iostream>
#include <sstream>
#include <cmath>

using namespace std;


int maxValuation = 4;
sf::Color valuationColors[] = {
    sf::Color::Red,
    sf::Color(255,128,0,255),
    sf::Color::Yellow,
    sf::Color::Green,
    sf::Color::Green
};

vector<vector<string> > valuations;

void initValuationNames()
{
    for (int i = 0; i < maxValuation+1; i++)
        valuations.push_back(vector<string>());
    valuations[0].push_back("ACQUIHIRED");
    valuations[0].push_back("DREAMS CRUSHED");
    valuations[1].push_back("REALITY SETS IN");
    valuations[1].push_back("2-MONTH RUNWAY");
    valuations[2].push_back("USELESS PRODUCT");
    valuations[2].push_back("PIVOT");
    valuations[2].push_back("REFUTED BY SCIENCE");
    valuations[3].push_back("CTO LEFT");
    valuations[4].push_back("SERIES A");
    valuations[4].push_back("YCOMBINATOR");
}
    

float strike(float t)
{
    if (t < 0.25)
        return t/0.25;
    if (t < 0.5)
        return 1-(t-0.25)/0.5;
    return 0.5 - (t-.5)/(1-.5) * .5;
}

float sign(float x)
{
    if (x > 0.f)
        return 1;
    return -1;
}

sf::Vector2f normalize(sf::Vector2f v)
{
    float r = sqrt(v.x*v.x+v.y*v.y);
    if (r < 0.05f)
        return sf::Vector2f(0.f,0.f);
    return sf::Vector2f(v.x/r, v.y/r);
}

float clamp(float min, float max, float v)
{
    if (v > max) return max;
    if (v < min) return min;
    return v;
}

float getJoyX(int id)
{
    float v;
    if (id == 0)
        v = sf::Joystick::getAxisPosition(0, sf::Joystick::X);
    else
        v = sf::Joystick::getAxisPosition(0, sf::Joystick::U);
    v = sign(v) * fmax(0.f, fabs(v)-20.f) * 1.25f;
    return v/100.f;
}
float getJoyY(int id)
{
    float v;
    if (id == 0)
        v = sf::Joystick::getAxisPosition(0, sf::Joystick::Y);
    else
        v = sf::Joystick::getAxisPosition(0, sf::Joystick::V);
    v = sign(v) * fmax(0.f, fabs(v)-20.f) * 1.25f;
    return v/100.f;
}

bool getJumpButton(int id)
{
    if (id == 1) return false;
    return sf::Joystick::isButtonPressed(0, 2);
}

bool getDodgeButton()
{
    return sf::Joystick::isButtonPressed(0, 3);
}

float getAttackButton()
{
    return sf::Joystick::isButtonPressed(0, 0);
}

float getGrabButton()
{
    return sf::Joystick::isButtonPressed(0, 1);
}

enum Direction { None, Left, Right };

Direction getLungeButton()
{
    if (sf::Joystick::isButtonPressed(0, 4))
        return Left;
    if (sf::Joystick::isButtonPressed(0, 5))
        return Right;
    return None;
}

string joystickState()
{
    bool connected = sf::Joystick::isConnected(0);
    unsigned int buttons = sf::Joystick::getButtonCount(0);
    bool hasX = sf::Joystick::hasAxis(0, sf::Joystick::X);
    bool pressed = sf::Joystick::isButtonPressed(0, 2);
    float position = sf::Joystick::getAxisPosition(0, sf::Joystick::Y);
    std::ostringstream s;

    for (int c = 0; c < buttons; c++) {
        s << "button " << c << " " << sf::Joystick::isButtonPressed(0, c) << endl;
    }
    s << "attack " << getAttackButton() << endl;
    s << "grab " << getGrabButton() << endl;
    return s.str();
}
string joystickPositions()
{
    std::ostringstream s;
    s << "joystick: "
        << "position: " << sf::Joystick::getAxisPosition(0, sf::Joystick::X) << endl
        << "position: " << sf::Joystick::getAxisPosition(0, sf::Joystick::Y) << endl
        << "position: " << sf::Joystick::getAxisPosition(0, sf::Joystick::Z) << endl
        << "position: " << sf::Joystick::getAxisPosition(0, sf::Joystick::R) << endl
        << "position: " << sf::Joystick::getAxisPosition(0, sf::Joystick::U) << endl
        << "position: " << sf::Joystick::getAxisPosition(0, sf::Joystick::V) << endl;
    return s.str();
}

enum JumpState {
    Ground, Crouching, Floating
};
string JumpStates[] = {
    "Ground", "Crouching", "Floating"
};
enum LungeState {
    Ready, Striking_Left, Striking_Right, Recovery
};
string LungeStates[] = {
    "Ready", "Striking_Left", "Striking_Right", "Recovery"
};
enum AttackState {
    AS_Ready, AS_Striking, AS_Grabbing
};
string AttackStates[] = {
    "AS_Ready", "AS_Striking", "AS_Grabbing"
};
enum MotionState {
    MS_Free, MS_Struck
};
string MotionStates[] = {
    "MS_Free", "MS_Struck"
};
// todo needed?
//enum JumpEdge {
//    StopJump, StartJump, NoEdge
//};

const int fps = 60;
const float groundLevel = 750.;

class ControlState
{
public:
    int id;
    JumpState jump;
    LungeState lunge;
    AttackState attack;
    MotionState motion;

    int facing; // TODO

    // Main state
    sf::Vector2f p;
    sf::Vector2f v;
    float radius;
    int valuation;
    string valuationLabel;

    sf::Vector2f arm;

    // State machine timing state
    int crouchDuration;
    int lungeDuration;
    int attackDuration;
    int struckDuration;
    sf::Vector2f attackStart;
    sf::Vector2f attackEnd;
    // State machine parameters
    const static float maxCrouch = fps/10;
    const static float maxLunge = fps/12;
    const static float maxRecovery = fps/6;
    const static float maxAttack = fps*5/6;
    const static float maxStruck = fps/4;

    // General Parameters
    const static float maxVertical = 5000;
    const static float maxHorizontal = 3500;
    const static float maxDodgeHorizontal = 7000;
    const static float gravity = 15000.f;
    const static float jumpForce = 25000.f;
    const static float attackRange = 800.f;
    const static float strikeForce = 1820.f;


    ControlState()
    {
        resetJump();
        jump = Floating;
        resetLunge();
        resetAttack();
        resetMotion();

        radius = 100.;
        p.x = 0.f;
        p.y = 0.f;
        v.x = 0.f;
        v.y = 0.f;
        valuation = maxValuation;
        updateValuationLabel();
    }

    void initX(int max)
    {
        if (id == 0)
            p.x = 100.f + radius;
        else
            p.x = max - 100.f - radius;
    }
    void resetJump()
    {
        jump = Ground;
        crouchDuration = 0;
    }
    void resetLunge()
    {
        lunge = Ready;
        lungeDuration = 0;
    }
    void resetAttack()
    {
        attack = AS_Ready;
        attackDuration = 0;
    }
    void resetMotion()
    {
        motion = MS_Free;
        struckDuration = 0;
    }

    void initiateAttack()
    {
        sf::Vector2f vec(getJoyX(id), getJoyY(id));
        float r = sqrt(vec.x*vec.x+vec.y*vec.y);
        attackStart = p;
        if (r < 0.05f)
            attackEnd = attackStart;
        else
            attackEnd = attackStart + vec / r * attackRange;
    }

    sf::Vector2f getCenter()
    {
        return p;
    }

    void stateUpdate()
    {
        // Update jump
        if (getJumpButton(id)) {
            if (jump == Ground) {
                jump = Crouching;
                crouchDuration = 0;
            }
            if (jump == Crouching && crouchDuration > maxCrouch) {
                jump = Floating;
            }
        } else {
            if (jump == Crouching)
                jump = Floating;
        }
        crouchDuration++;

        // Update lunge
        Direction action = getLungeButton();
        switch (lunge) {
            case Ready:
                if (action == Left)
                    lunge = Striking_Left;
                if (action == Right)
                    lunge = Striking_Right;
                lungeDuration = 0;
                break;
            case Striking_Left:
            case Striking_Right:
                if (lungeDuration > maxLunge)
                    lunge = Recovery;
                break;
            case Recovery:
                if (lungeDuration > maxRecovery)
                    resetLunge();
                break;
        }
        lungeDuration++;

        // Update attack
        switch (attack) {
            case AS_Ready:
                if (getAttackButton()) {
                    attack = AS_Striking;
                }
                if (getGrabButton()) {
                    attack = AS_Grabbing;
                }
                initiateAttack();
                attackDuration = 0;
                arm = getCenter();
                break;
            case AS_Striking:
            case AS_Grabbing:
                if (attackDuration > maxAttack)
                    resetAttack();
                else {
                    float t = (float)attackDuration / maxAttack;
                    float r = strike(t);
                    sf::Vector2f p1 = attackStart * (1.f - r) + attackEnd * r;
                    sf::Vector2f p2 = p1 * (1.f - t) + p * t;
                    arm = p2;
                }
                break;
        }
        attackDuration++;

        switch (motion) {
            case MS_Free:
                struckDuration = 0;
                break;
            case MS_Struck:
                if (struckDuration++ > maxStruck) {
                    resetMotion();
                }
                break;
        }

    }
    void updateVelocity(float dt)
    {
        sf::Vector2f newV = v;
        // Horizontal joystick
        float dvx = getJoyX(id) * 500.f;
        if (jump == Crouching || jump == Floating)
            dvx /= 3;
        if (motion == MS_Struck)
            dvx /= 3;

        newV.x += dvx;
        if (motion == MS_Free)
            newV.x = clamp (-maxHorizontal, maxHorizontal, newV.x);

        if (motion == MS_Free) {
            switch(jump) {
                //case Ground:
                //    if (lunge == Ready) {
                //        //newV.x = getJoyX() * 2500;
                //        facing = sign(newV.x);
                //    }
                //    //vy -= gravity * dt;
                //    newV.y = 0.;
                //    break;
                case Crouching:
                    newV.y += jumpForce * dt;
                    break;
            }
        }

        newV = applyGravity(newV, dt);

        switch(lunge) {
            case Ready:
                break;
            case Striking_Left:
                if (motion == MS_Free)
                    newV.x = -maxDodgeHorizontal * strike((float)lungeDuration/maxRecovery);
                break;
            case Striking_Right:
                if (motion == MS_Free)
                    newV.x = maxDodgeHorizontal * strike((float)lungeDuration/maxRecovery);
                break;
            case Recovery:
                //vx = sign(vx) * maxHorizontal * strike(lungeDuration/maxRecovery);
                break;
        }

        switch (motion) {
            case MS_Free:
                v = newV;
                v.x *= 0.9; // todo
                break;
            case MS_Struck:
                v = newV;
                break;
        }
    }

    sf::Vector2f applyGravity(sf::Vector2f v, float dt)
    {
        if ((jump == Floating || jump == Ground) && motion == MS_Free)
            v.y -= gravity * dt;
        if (lunge == Striking_Right || lunge == Striking_Left)
            v.y = 0.;
        return v;
    }

    void update(float dt)
    {
        stateUpdate();
        updateVelocity(dt);
        p.x += dt * v.x;
        p.y -= dt * v.y;

        if (p.y > groundLevel) {
            resetJump();
            p.y = groundLevel;
            v.y = 0;
        }

    }


    string toString(float time)
    {
        std::ostringstream s;
        s << JumpStates[jump] << endl << LungeStates[lunge] << endl
            << AttackStates[attack] << endl
            << MotionStates[motion] << endl;
        return s.str();
    }

    void updateValuationLabel()
    {
        valuationLabel = valuations[valuation][rand() % valuations[valuation].size()];
    }
    void doHit(ControlState &target)
    {
        if (target.motion == MS_Struck)
            return;
        sf::Vector2f v = arm - p;
        v = normalize(v) * strikeForce;
        v.y = -v.y;
        switch(attack) {
            case AS_Ready:
                return;
            case AS_Striking:
                v += sf::Vector2f(0, 1200.f);
                break;
            case AS_Grabbing:
                v = -v;
                break;
        }
        target.motion = MS_Struck;
        target.valuation--;
        target.updateValuationLabel();
        if (target.valuation < 0) target.valuation = 0;
        target.v = v;
    }
};

bool collision(ControlState &s1, ControlState &s2)
{
    sf::Vector2f v;
    // P2 hits P1
    v = s1.p - s2.arm;
    if (v.x*v.x + v.y*v.y < s1.radius*s1.radius)
    {
        s2.doHit(s1);
    }
    // P1 hits P2
    v = s2.p - s1.arm;
    if (v.x*v.x + v.y*v.y < s2.radius*s2.radius)
    {
        s1.doHit(s2);
    }
    return false;
}

class Char : public sf::Drawable
{
public:
    sf::Vector2u windowDim;

    Char(int myid, sf::Vector2u dim, bool showText)
    {
        controller.id = myid;

        // Graphics state
        if (!s_font.loadFromFile("noto.ttf"))
            exit(22);
        m_text.setFont(s_font);
        m_text.setCharacterSize(36);
        m_text.setColor(sf::Color(0,255,0,255));
        windowDim = dim;

        if (!m_texture.loadFromFile("pt-crop.jpg"))
            exit(23);
        m_sprite.setTexture(m_texture);
        m_sprite.setScale(0.25f, 0.25f);
        m_sprite.setPosition(300, 300);

        // Game state
        controller.initX(dim.x);
    }
    virtual ~Char()
    {
    }
    void update(float time)
    {
        float dt = 1.0/60;

        controller.update(dt);

        m_sprite.setPosition(controller.p.x, controller.p.y);

        m_text.setColor(valuationColors[controller.valuation]);
        m_text.setString(controller.valuationLabel);
        sf::FloatRect bounds = m_text.getLocalBounds();
        float x;
        if (controller.id == 0)
            x = 70;
        else
            x = windowDim.x - bounds.width - 70;
        m_text.setPosition(x, windowDim.y-150.f);
        

        std::ostringstream s;
        if (dt > 0.018f) {
            cout << dt << endl;
            exit(20);
        }
        s   << 1./dt << endl
            << joystickState() << endl
            << state() << endl
            << controller.toString(time) << endl;

        //m_text.setString(s.str());
    }
    void drawArm(sf::RenderTarget& target, sf::RenderStates states) const
    {
        float x = controller.arm.x;
        float y = controller.arm.y;
        float r = 22.f;
        sf::CircleShape shape(r);
        if (controller.attack == AS_Ready)
            shape.setFillColor(sf::Color(0,204,102,255));
        else if (controller.attack == AS_Striking)
            shape.setFillColor(sf::Color(255,0,0,255));
        else if (controller.attack == AS_Grabbing)
            shape.setFillColor(sf::Color(255,128,0,255));
        shape.setPosition(x-r, y-r);
        target.draw(shape);
    }
    void drawStatus(sf::RenderTarget& target, sf::RenderStates states) const
    {
    }
    void draw(sf::RenderTarget& target, sf::RenderStates states) const
    {
        target.draw(m_text, states);
        //target.draw(m_sprite);
        float x = controller.p.x;
        float y = controller.p.y;
        float r = controller.radius;
        sf::CircleShape shape(r);
        shape.setFillColor(sf::Color(0,222,0,128));
        shape.setPosition(x-r, y-r);
        target.draw(shape);

        drawArm(target, states);
    }

    ControlState controller;

    sf::Font s_font;
    sf::Text m_text;

    sf::Texture m_texture;
    sf::Sprite m_sprite;

    string state()
    {
        std::ostringstream s;
        s
            << "x: " << controller.p.x << endl
            << "y: " << controller.p.y << endl
            << "vx: " << controller.v.x << endl
            << "vy: " << controller.v.y << endl;
        return s.str();
    }

    //static const sf::Font* s_font;
    //static void setFont(const sf::Font& font)
    //{
    //    s_font = &font;
    //}
};

void mainUpdate(Char &t1, Char &t2)
{
    t1.update(0);
    t2.update(0);
    // updateoverlay
    collision(t1.controller, t2.controller);
}

int main()
{
    sf::RenderWindow window(sf::VideoMode(200, 200), "SFML works!");
    srand(time(0));

    //sf::CircleShape shape(100.f);
    //shape.setFillColor(sf::Color::Green);

    initValuationNames();

    sf::Vector2u dim = window.getSize();

    sf::RectangleShape ground;
    ground.setSize(sf::Vector2f(dim.x, 22));
    ground.setPosition(0.,groundLevel);
    ground.setFillColor(sf::Color(255,128,0,255));

    //for (float f = 0.; f < 1.; f += 0.01)
    //    cout << strike(f) << endl;
    //exit(0);

    int c = 0;

    window.setFramerateLimit(fps);

    float offset = 200.f;
    Char thing(0, dim, true);
    Char thing2(1, dim, false);

    sf::Clock clock;
    thing.update(clock.getElapsedTime().asSeconds());
    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            //std::cout << c++ << "!\n";
            if (event.type == sf::Event::Closed)
                window.close();

        }
        mainUpdate(thing, thing2);
        //clock.getElapsedTime().asSeconds();

        window.clear();
        window.draw(thing);
        window.draw(thing2);
        //window.draw(ground);
        window.display(); // this tries to enforce the frame limit
    }
    window.close();
    return 0;
}
