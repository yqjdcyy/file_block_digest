

class DiskSHA1Encoder{

    static{
        System.loadLibrary("DiskSHA1Encoder");
    }

    private native String encode(String fileUrl);

    public static void main(String[] args){
        System.out.println(new DiskSHA1Encoder().encode(""));
    }
}