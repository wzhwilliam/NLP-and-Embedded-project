package WDM.pojo;

import lombok.Data;

@Data
public class Item {
    long orderId;
    long itemId;
    double price;
    int amount;
}

