package WDM.utils;

import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;

public class ResponseCode {

    public ResponseEntity ok(){
        return new ResponseEntity(HttpStatus.OK);
    }

    public ResponseEntity error(){
        return new ResponseEntity(HttpStatus.BAD_REQUEST);
    }


}
