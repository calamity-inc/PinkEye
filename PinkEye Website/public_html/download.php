<?php
        $rate_limit = array(
            'time_interval' => 60,
            'max_requests' => 7,
            'clear_duration' => 3600,
        );
        
        $user_ip = $_SERVER['REMOTE_ADDR'];
        $current_time = time();
        
        $data_file = '../rate_limit_data.txt';
        $data = file_get_contents($data_file);
        $rate_limit_data = array();
        if ($data !== false) {
            $rate_limit_data = unserialize($data);
        }
        
        if (!isset($rate_limit_data[$user_ip])) {
            $rate_limit_data[$user_ip] = array(
                'last_request_time' => 0,
                'request_count' => 0,
                'clear_time' => $current_time + $rate_limit['clear_duration'],
            );
        }
        
        $last_request_time = $rate_limit_data[$user_ip]['last_request_time'];
        $request_count = $rate_limit_data[$user_ip]['request_count'];
        $clear_time = $rate_limit_data[$user_ip]['clear_time'];
        
        if ($current_time >= $clear_time) {
            $request_count = 0;
            $clear_time = $current_time + $rate_limit['clear_duration'];
        }
        if ($current_time - $last_request_time > $rate_limit['time_interval']) {
            $request_count = 0;
        }
        
        $rate_limit_data[$user_ip]['last_request_time'] = $current_time;
        $rate_limit_data[$user_ip]['request_count'] = $request_count + 1;
        $rate_limit_data[$user_ip]['clear_time'] = $clear_time;
        
        file_put_contents($data_file, serialize($rate_limit_data));
        
        if ($request_count >= $rate_limit['max_requests']) {
            header('HTTP/1.1 429 Too Many Requests');
            header('Content-Type: text/html');
            echo '<script>alert("You have exceeded the rate limit. Please try again later.");</script>';
            echo '<script>window.location = window.location.protocol + "//" + window.location.hostname;</script>';
            exit();
        }
        
        if ($_SERVER['REQUEST_METHOD'] === 'POST') {
			$license_key = $_POST['license_key'];
			$found_key = false;
			
			$url_pk = "https://backend-services.stand.sh/internal_check_pinkeye/?0=" . str_replace("Stand-Activate-", "", $license_key);
			$response_pk = file_get_contents($url_pk);
			$data = json_decode($response_pk, true);
			    
			if ($response_pk !== false)
			{
			    if ($data['dev'] === true || $data['pinkeyed'] === true)
			    {
			        $found_key = true;
			    }
			}
			
			if ($found_key)
			{
				$file_name = 'PinkEye V5.rar';
				header('Content-Type: application/octet-stream');
                header('Content-Disposition: attachment; filename="' . $file_name . '"');
                readfile('../PinkEye.rar');
				exit();
			}
			else
			{
				echo '<script>alert("Wrong License Key!");</script>';
				echo '<script>window.location = window.location.protocol + "//" + window.location.hostname;</script>';
				exit();
			}
		}
    ?>
<!DOCTYPE html>
<html>
  <head>
    <title>License Key Verification</title>
    <style>
      body {
        background-color: #121212;
        font-family: Arial, sans-serif;
        font-size: 14px;
        line-height: 1.5;
        margin: 0;
        padding: 0;
      }
      #license-form {
        background-color: #fff;
        border-radius: 5px;
        box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1);
        margin: 20px auto;
        padding: 20px;
        text-align: center;
        width: 400px;
      }
      #license-form label {
        display: block;
        margin-bottom: 10px;
        text-align: left;
        font-weight: bold;
      }
      #license-form input[type="text"] {
        border: 2px solid #ddd;
        border-radius: 3px;
        box-sizing: border-box;
        font-size: 16px;
        padding: 10px;
        width: 100%;
      }
      #license-form input[type="submit"] {
        background-color: #FF1493;
        border: none;
        border-radius: 3px;
        box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1);
        color: #fff;
        cursor: pointer;
        font-size: 16px;
        padding: 10px 20px;
        margin-top: 10px;
        transition: background-color 0.2s ease;
      }
      #license-form input[type="submit"]:hover {
        background-color: #ff47a8;
      }
    </style>
  </head>
  <body>
    <div id="license-form">
      <form method="post">
        <label for="license_key">Sharing the PinkEye download or any PinkEye files is against the TOS and is 100% bannable!<br><br>Enter Stand Activation Key:</label>
        <input type="text" id="license_key" name="license_key" placeholder="Stand-Activate-XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" required>
        <input type="submit" value="Submit">
      </form>
    </div>
  </body>
</html>