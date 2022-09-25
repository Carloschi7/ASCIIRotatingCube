#include <iostream>
#include <cstring>
#include <cmath>
#include <thread>
#include <chrono>

//Utility classes for point rotation
template <uint32_t ROWS, uint32_t COLS>
class SimpleMatrix
{
public:
    SimpleMatrix(float data[ROWS][COLS])
    {
        memcpy(m_Data, data, sizeof(float) * ROWS * COLS);
    }

    void PrintMatrix()
    {
        for(int i = 0; i < ROWS; i++)
        {
            for(int j = 0; j < COLS; j++)
            {
                std::cout << m_Data[i][j];
            }
            std::cout << std::endl;
        }
    }

    const float& elem(uint32_t x, uint32_t y) const
    {
        if(x >= ROWS || y >= COLS)
        {
            throw std::runtime_error("Indexes out of bounds");
        }

        return m_Data[x][y];

    }

    float& elem(uint32_t x, uint32_t y)
    {
        if(x >= ROWS || y >= COLS)
        {
            throw std::runtime_error("Indexes out of bounds");
        }

        return m_Data[x][y];

    }

    SimpleMatrix operator*(const SimpleMatrix<COLS, ROWS>& other)
    {
        float data[3][3];

        for(int i = 0; i < ROWS; i++)
        {
            for(int j = 0; j < COLS; j++)
            {
                data[i][j] = 0;
                for(int k = 0; k < COLS; k++)
                {
                    data[i][j] += this->elem(i, k) * other.elem(k, j);
                }
            }
        }

        return {data};
    }
private:
    float m_Data[ROWS][COLS]{0};
};

//Basic rotations
//Default rotation matrices in all coordiantes

SimpleMatrix<3,3> Rx(float fAngle)
{
    float data[3][3]{{1,                    0,                  0                   },
                     {0,                    std::cos(fAngle),   -std::sin(fAngle)   },
                     {0,                    std::sin(fAngle),   std::cos(fAngle)    } };

    return {data};
}

SimpleMatrix<3,3> Ry(float fAngle)
{
    float data[3][3]{{std::cos(fAngle),     0,                  std::sin(fAngle)    },
                     {0,                    1,                  0                   },
                     {-std::sin(fAngle),    0,                  std::cos(fAngle)    } };

    return {data};
}

SimpleMatrix<3,3> Rz(float fAngle)
{
    float data[3][3]{{std::cos(fAngle),     -std::sin(fAngle),  0                   },
                     {std::sin(fAngle),     std::cos(fAngle),   0                   },
                     {0,                    0,                  1                   } };

    return {data};
}

struct Vector3D
{
    float x = 0, y = 0, z = 0;

    //Basic multiplication with a matrix
    Vector3D operator*(const SimpleMatrix<3,3>& mat) const
    {
        Vector3D output;
        output.x = mat.elem(0, 0) * x + mat.elem(0,1) * y + mat.elem(0,2)* z;
        output.y = mat.elem(1, 0) * x + mat.elem(1,1) * y + mat.elem(1,2)* z;
        output.z = mat.elem(2, 0) * x + mat.elem(2,1) * y + mat.elem(2,2)* z;
        return output;
    }

    void PrintVector()
    {
        std::cout << x << ", " << y << ", " << z << std::endl;
    }

    Vector3D operator-() const
    {
        return {-x, -y, -z};
    }

};

//Actual program
static const uint32_t width = 130, height = 50;
char screen_buf[width * height];
float fAttenuator = 0.1f;
float fDistanceFromCamera = 2.0f;
float fIncrement = 0.02f;
//Half a cube's diagonal
float fClosestDepth = std::sqrt(0.75f);

//Empiric space -> Rotated space
Vector3D Multiply(const Vector3D& cube_pos, const Vector3D& rotation_angle)
{
    auto ToRadians = [](float fAngle){return fAngle * (M_PI / 180.0f);};
    float fRadX = ToRadians(rotation_angle.x);
    float fRadY = ToRadians(rotation_angle.y);
    float fRadZ = ToRadians(rotation_angle.z);
    Vector3D output = cube_pos * (Rx(fRadX) * Ry(fRadY) * Rz(fRadZ));
    return output;
}

//Checking only for cube edges
bool Condition1(float x, float y, float z)
{
    return (x == -0.5f && y == -0.5f) ||
           (x == -0.5f && z == -0.5f) ||
           (y == -0.5f && z == -0.5f) ||
            (x + fIncrement >= 0.5f && y + fIncrement >= 0.5f) ||
            (x + fIncrement >= 0.5f && z + fIncrement >= 0.5f) ||
            (y + fIncrement >= 0.5f && z + fIncrement >= 0.5f)||
            (x + fIncrement >= 0.5f && y == -0.5f) ||
            (x + fIncrement >= 0.5f && z == -0.5f) ||
            (y + fIncrement >= 0.5f && z == -0.5f) ||
            (y + fIncrement >= 0.5f && x == -0.5f) ||
            (z + fIncrement >= 0.5f && x == -0.5f) ||
            (z + fIncrement >= 0.5f && y == -0.5f);
}

uint32_t Condition(float x, float y, float z)
{
    if(x == -0.5f)
        return 1;
    if(x + fIncrement >= 0.5f)
        return 2;
    if(y == -0.5f)
        return 3;
    if(y + fIncrement >= 0.5f)
        return 4;
    if(z == -0.5f)
        return 5;
    if(z + fIncrement >= 0.5f)
        return 6;

    return 0;
}

int32_t GetSymPriority(char c)
{
	switch(c)
	{
	case '#':
		return 4;
	case '+':
		return 3;
	case '-':
		return 2;
	case '.':
		return 1;
	case ' ':
		return 0;
	default:
		return -1;
	}
}

void SetBuf(char& buf_tile, char input)
{
	if(GetSymPriority(input) > GetSymPriority(buf_tile))
		buf_tile = input;
}

int main()
{
    //A more mathematical approximation
    auto approx = [](float value)
        {
            float cut = value - uint32_t(value);
            if(cut > 0.5f)
                return uint32_t(value) + 1;
            else
                return uint32_t(value);
        };

    float fAngle = 0.0f;
    for(;;)
    {
        memset(screen_buf, ' ', width * height);
	    fAngle += 0.5f;
        Vector3D rotation_angle = {fAngle, fAngle / 2.0f, fAngle / 8.0f};

        for(float x = -0.5f; x < 0.5f; x += fIncrement)
        {
            for(float y = -0.5f; y < 0.5f; y += fIncrement)
            {
                for(float z = -0.5f; z < 0.5f; z += fIncrement)
                {
                    uint32_t res = Condition(x,y,z);
                    Vector3D rotated_norm;
                    switch(res)
                    {
                        case 1:
                            rotated_norm = Multiply({-1.0f, 0.0f, 0.0f}, rotation_angle);
                            break;
                        case 2:
                            rotated_norm = Multiply({1.0f, 0.0f, 0.0f}, rotation_angle);
                            break;
                        case 3:
                            rotated_norm = Multiply({0.0f, -1.0f, 0.0f}, rotation_angle);
                            break;
                        case 4:
                            rotated_norm = Multiply({0.0f, 1.0f, 0.0f}, rotation_angle);
                            break;
                        case 5:
                            rotated_norm = Multiply({0.0f, 0.0f, -1.0f}, rotation_angle);
                            break;
                        case 6:
                            rotated_norm = Multiply({0.0f, 0.0f, 1.0f}, rotation_angle);
                            break;
                        default:
                            break;
                    }

                    auto dot_product = [](const Vector3D& a, const Vector3D& b)
                            {return a.x*b.x + a.y*b.y + a.z*b.z;};

                    if(res)
                    {
                        Vector3D transformed = Multiply({x,y,z}, rotation_angle);
                        transformed.z += fDistanceFromCamera;
                        //Rotated space -> screen space
                        uint32_t lx = approx(width/2 + (transformed.x * width/2) / transformed.z);
                        uint32_t ly = approx(height/2 - (transformed.y * height/2) / transformed.z);

                        Vector3D view_direction{0.0f, 0.0f, 1.0f};
                        float fBrightness = (dot_product(-view_direction, rotated_norm) + 1.0f) / 2.0f;

                        if(fBrightness < 0.7f)
                            SetBuf(screen_buf[ly * width + lx], '.');
                        else if(fBrightness >= 0.7f && fBrightness < 0.75f)
                            SetBuf(screen_buf[ly * width + lx], '-');
                        else if(fBrightness >= 0.75f && fBrightness < 0.9f)
                            SetBuf(screen_buf[ly * width + lx], '+');
                        else if(fBrightness >= 0.9f)
                            SetBuf(screen_buf[ly * width + lx], '#');
                    }
                }
            }
        }

        for(int i = 0; i < height; i++)
        {
            for(int k = 0; k < width; k++)
            {
                std::cout << screen_buf[i * width + k];
            }
            std::cout << std::endl;
        }

        //Sleeping and clearing
	    std::this_thread::sleep_for(std::chrono::milliseconds(15));
        printf("\033[H\033[J");
        printf("\033[%d;%dH", 0, 0);
    }

	return 0;
}
