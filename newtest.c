int multiply(int x, int y){
    return x * y;
}

int main(){
    int x = 3;
    int y = 4;
    x+=y;
    y+=x;
    int answer = multiply(x,y);
}